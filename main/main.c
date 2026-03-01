#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_tls_crypto.h"
#include <esp_http_server.h>
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "esp_check.h"

#if !CONFIG_IDF_TARGET_LINUX
#include <esp_wifi.h>
#include <esp_system.h>
#include "nvs_flash.h"
#include "esp_eth.h"
#include "esp_spiffs.h"
#endif  // !CONFIG_IDF_TARGET_LINUX
#include "driver/ledc.h"
#include "driver/gpio.h"
#define EXAMPLE_HTTP_QUERY_KEY_MAX_LEN  (64)
static const char *TAG = "ws_echo_server";
static const char *TAGMOTOR ="motor_msg";
// PWM配置
#define PWM_FREQUENCY 1000    // 1kHz
#define PWM_RESOLUTION LEDC_TIMER_8_BIT  // 8位分辨率 (0-255)
#define PWM_DUTY_CYCLE 200   // 固定占空比，大约78%速度
#define MOTOR_AIN1  GPIO_NUM_25
#define MOTOR_AIN2  GPIO_NUM_26
#define MOTOR_BIN1  GPIO_NUM_27
#define MOTOR_BIN2  GPIO_NUM_14
#define MOTOR_PWMA  GPIO_NUM_12  // PWM通道
#define MOTOR_PWMB  GPIO_NUM_13  // PWM通道
#define MOTOR_STBY  GPIO_NUM_2
void motor_gpio_init() {
    // 配置GPIO为输出模式
    gpio_config_t io_conf = {};
    
    // 设置GPIO位掩码
    io_conf.pin_bit_mask = (1ULL << MOTOR_AIN1) | 
                           (1ULL << MOTOR_AIN2) | 
                           (1ULL << MOTOR_BIN1) | 
                           (1ULL << MOTOR_BIN2) | 
                           (1ULL << MOTOR_STBY);
    
    // 设置为输出模式
    io_conf.mode = GPIO_MODE_OUTPUT;
    // 禁用上拉下拉
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // 禁用中断
    io_conf.intr_type = GPIO_INTR_DISABLE;
    
    // 应用配置
    gpio_config(&io_conf);
    
    // 初始状态：STBY拉高，其他拉低
    gpio_set_level(MOTOR_STBY, 1);
    gpio_set_level(MOTOR_AIN1, 0);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN1, 0);
    gpio_set_level(MOTOR_BIN2, 0);
}
void motor_pwm_init() {
    // 定时器配置
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);
    
    // PWMA通道配置
    ledc_channel_config_t pwma_conf = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = MOTOR_PWMA,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&pwma_conf);
    
    // PWMB通道配置（类似）
    // ...
        ledc_channel_config_t pwmb_conf = {
        .channel = LEDC_CHANNEL_1,
        .duty = 0,
        .gpio_num = MOTOR_PWMB,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0
    };
    ledc_channel_config(&pwmb_conf);
}
void motor_stop(){
    ESP_LOGI(TAGMOTOR,"Motor forward");
    gpio_set_level(MOTOR_AIN1, 1);
    gpio_set_level(MOTOR_AIN2, 1);
    gpio_set_level(MOTOR_BIN1, 1);
    gpio_set_level(MOTOR_BIN2, 1);
    gpio_set_level(MOTOR_STBY, 1);
}
void motor_forward() {
    ESP_LOGI(TAGMOTOR,"Motor forward");
    gpio_set_level(MOTOR_AIN1, 1);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN1, 1);
    gpio_set_level(MOTOR_BIN2, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    gpio_set_level(MOTOR_STBY, 1);
    
    
    // 启动PWM
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, PWM_DUTY_CYCLE);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, PWM_DUTY_CYCLE);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    // PWMB同理
}

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

/*
 * async send function, which we put into the httpd work queue
 */
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

/*
 * This handler echos back the received ws data
 * and triggers an async send if certain message received
 */
static esp_err_t echo_handler(httpd_req_t *req)
{
    // 检查是否是WebSocket握手请求
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "握手请求，升级到WebSocket");
        ESP_LOGI(TAG, "收到请求，方法: %d, URI: %s", req->method, req->uri);
        return ESP_OK; // 框架自动处理握手
    }
    
    // 接收WebSocket消息
    httpd_ws_frame_t ws_pkt;
    uint8_t buf[128] = {0};
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = buf;
    
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 128);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "接收WebSocket帧失败");
        return ret;
    }

    // 解析控制命令
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
        ESP_LOGI(TAG, "收到命令: %s", (char*)buf);
        
        if (strcmp((char*)buf, "forward_start") == 0) {
            ESP_LOGI(TAG,"forward");
            motor_forward();
        } else if (strcmp((char*)buf, "backward_start") == 0) {
            ESP_LOGI(TAG,"backward");
            //motor_backward();
        } else if (strcmp((char*)buf, "left_start") == 0) {
            ESP_LOGI(TAG,"turn left");
            //motor_turn_left();
        } else if (strcmp((char*)buf, "right_start") == 0) {
            ESP_LOGI(TAG,"turn right");
            //motor_turn_right();
        } else if (strcmp((char*)buf, "stop") == 0) {
            ESP_LOGI(TAG,"stop");
            motor_stop();
        } else if (strcmp((char*)buf, "rotate_step") == 0) {
            ESP_LOGI(TAG, "rotate step");
            // 原地转一小步的电机控制
        } else if (strcmp((char*)buf, "turn_left") == 0) {
            ESP_LOGI(TAG, "turn left");
            // 左转
        } else if (strcmp((char*)buf, "turn_right") == 0) {
            ESP_LOGI(TAG, "turn right");
            // 右转
        }
        
        
        // 可选：回复确认消息
        char reply[256];
        sprintf(reply, "执行: %s", (char*)buf);
        httpd_ws_frame_t reply_pkt;
        memset(&reply_pkt, 0, sizeof(httpd_ws_frame_t));
        reply_pkt.payload = (uint8_t*)reply;
        reply_pkt.len = strlen(reply);
        reply_pkt.type = HTTPD_WS_TYPE_TEXT;
        httpd_ws_send_frame(req, &reply_pkt);        
    }
    return ESP_OK;
}

static const httpd_uri_t ws = {
        .uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = echo_handler,
        .user_ctx   = NULL,
        .is_websocket = true
};

// mount SPIFFS
void init_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = "storage",
      .max_files = 5,
      .format_if_mount_failed = true
    };
    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));

    size_t total = 0, used = 0;
    ESP_ERROR_CHECK(esp_spiffs_info(conf.partition_label, &total, &used));
    ESP_LOGI("SPIFFS", "Partition size: total: %d, used: %d", total, used);
}
//network part

esp_err_t root_get_handler(httpd_req_t *req)
{
    FILE* f = fopen("/spiffs/index.html", "r");
    if (f == NULL) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    char buf[512];
    size_t read_bytes;
    httpd_resp_set_type(req, "text/html");

    while ((read_bytes = fread(buf, 1, sizeof(buf), f)) > 0) {
        httpd_resp_send_chunk(req, buf, read_bytes);
    }
    httpd_resp_send_chunk(req, NULL, 0);
    fclose(f);
    return ESP_OK;
}

httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
#if CONFIG_IDF_TARGET_LINUX
    // Setting port as 8001 when building for Linux. Port 80 can be used only by a privileged user in linux.
    // So when a unprivileged user tries to run the application, it throws bind error and the server is not started.
    // Port 8001 can be used by an unprivileged user as well. So the application will not throw bind error and the
    // server will be started.
    config.server_port = 8001;
#endif // !CONFIG_IDF_TARGET_LINUX
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &ws);
        #if CONFIG_EXAMPLE_BASIC_AUTH
        httpd_register_basic_auth(server);
        #endif
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}
static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}
static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

    ESP_LOGI(TAG, "Got IP:  " IPSTR, IP2STR(&event->ip_info.ip));

    if (*server == NULL) {
        *server = start_webserver();
    }
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver due to Wi-Fi disconnection");
        stop_webserver(*server);
        *server = NULL;
    }
}
/* -------------------- Wi-Fi 初始化 -------------------- */
#include "wifi_config.h"

static void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTHMODE,
        },
    };
    

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}
void app_main(void)
{
    static httpd_handle_t server = NULL;
    init_spiffs();
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_init_sta();
    motor_gpio_init();
    motor_pwm_init();
    // 注册事件回调
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
}