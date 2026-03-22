#include "http_server.h"
#include "command_handler.h"
#include "camera_stream.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_heap_caps.h"
#include <string.h>
#include <stdlib.h>

static const char *s_tag = "HTTP_SERVER";

static esp_err_t root_get_handler(httpd_req_t *req)
{
    FILE *f = fopen("/spiffs/index.html", "r");
    if (f == NULL) {
        ESP_LOGE(s_tag, "Failed to open index.html");
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    rewind(f);

    /* 整文件读入 PSRAM，单次 httpd_resp_send 发出（带 Content-Length）。
     * 避免 chunked 分块传输：中途 EAGAIN 会导致 chunk 边界损坏，
     * 浏览器报 ERR_INVALID_CHUNKED_ENCODING，脚本块被截断。
     * httpd_resp_send 内部的 httpd_send_all 会自动重试 EAGAIN。 */
    char *buf = heap_caps_malloc(file_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf) {
        buf = malloc(file_size);
    }
    if (!buf) {
        fclose(f);
        ESP_LOGE(s_tag, "malloc failed for index.html (%ld B)", file_size);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_FAIL;
    }

    size_t read_bytes = fread(buf, 1, file_size, f);
    fclose(f);

    httpd_resp_set_type(req, "text/html");
    esp_err_t ret = httpd_resp_send(req, buf, (ssize_t)read_bytes);
    heap_caps_free(buf);
    return ret;
}

static httpd_uri_t s_root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

static void ws_async_send(void *arg)
{
    static const char * data = "Async data";
    struct async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)data;
    ws_pkt.len = strlen(data);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req)
{
    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
    if (resp_arg == NULL) {
        return ESP_ERR_NO_MEM;
    }
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    esp_err_t ret = httpd_queue_work(handle, ws_async_send, resp_arg);
    if (ret != ESP_OK) {
        free(resp_arg);
    }
    return ret;
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(s_tag, "Handshake request, upgrading to WebSocket");
        return ESP_OK;
    }
    
    httpd_ws_frame_t ws_pkt;
    uint8_t buf[128] = {0};
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = buf;
    
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 128);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "WebSocket frame receive failed");
        return ret;
    }

    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
        ESP_LOGI(s_tag, "Received command: %s", (char*)buf);

        car_cmd_t cmd = command_parse((char*)buf);
        command_dispatch(cmd);
        
        char reply[256];
        sprintf(reply, "Executed: %s", (char*)buf);
        httpd_ws_frame_t reply_pkt;
        memset(&reply_pkt, 0, sizeof(httpd_ws_frame_t));
        reply_pkt.payload = (uint8_t*)reply;
        reply_pkt.len = strlen(reply);
        reply_pkt.type = HTTPD_WS_TYPE_TEXT;
        httpd_ws_send_frame(req, &reply_pkt);        
    }
    return ESP_OK;
}

static const httpd_uri_t s_ws = {
    .uri        = "/ws",
    .method     = HTTP_GET,
    .handler    = ws_handler,
    .user_ctx   = NULL,
    .is_websocket = true
};

/* ── /video：视频帧 WebSocket（二进制，服务器→客户端单向推流） ── */

static esp_err_t video_ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        ESP_LOGI(s_tag, "Video WebSocket client connected, fd=%d", fd);
        camera_stream_attach_client(req->handle, fd);
        return ESP_OK;
    }
    /* 视频流单向推送，忽略客户端发来的帧 */
    return ESP_OK;
}

static const httpd_uri_t s_video_ws = {
    .uri          = "/video",
    .method       = HTTP_GET,
    .handler      = video_ws_handler,
    .user_ctx     = NULL,
    .is_websocket = true,
};

esp_err_t init_spiffs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = "storage",
        .max_files = 5,
        .format_if_mount_failed = true
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "SPIFFS mount failed: %s", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "SPIFFS info failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(s_tag, "SPIFFS: total=%d, used=%d", total, used);
    return ESP_OK;
}

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
#if CONFIG_IDF_TARGET_LINUX
    config.server_port = 8001;
#endif
    
    config.lru_purge_enable    = true;
    config.stack_size          = 8192; /* 默认 4096 不够用于 WebSocket 帧发送 */
    config.send_wait_timeout   = 2;    /* 默认 5s 太长；TCP 拥塞时快速失败，避免 HTTPD 任务长期挂起 */

    ESP_LOGI(s_tag, "Starting server on port: '%d'", config.server_port);
    
    esp_err_t ret = httpd_start(&server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Server start failed: %s", esp_err_to_name(ret));
        return NULL;
    }

    ESP_LOGI(s_tag, "Registering URI handlers");
    httpd_register_uri_handler(server, &s_root);
    httpd_register_uri_handler(server, &s_ws);
    httpd_register_uri_handler(server, &s_video_ws);
    
    return server;
}

esp_err_t stop_webserver(httpd_handle_t server)
{
    if (server == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return httpd_stop(server);
}

void connect_handler(void* arg, esp_event_base_t event_base,
                     int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

    ESP_LOGI(s_tag, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

    /* 关闭 WiFi 省电模式：省电模式下 station 在 beacon 间隔内睡眠，
     * TCP ACK 被延迟最多 102ms，导致小文件传输也触发 EAGAIN。 */
    esp_wifi_set_ps(WIFI_PS_NONE);

    if (*server == NULL) {
        *server = start_webserver();
    }
}

void disconnect_handler(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(s_tag, "Stopping webserver due to Wi-Fi disconnection");
        stop_webserver(*server);
        *server = NULL;
    }
}
