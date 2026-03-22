#include "camera_stream.h"
#include "hal/hal_camera.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_heap_caps.h"
#include <stdlib.h>
#include <string.h>

/* ── 配置 ───────────────────────────────────────────── */
#define STREAM_TASK_STACK    8192
#define STREAM_TASK_PRIO     6
#define FRAME_INTERVAL_MS    200    /* ~5 fps；实际帧率受网速限制自动降低 */
#define MAX_INFLIGHT_FRAMES  1      /* 单帧背压：上一帧发完才发下一帧 */
#define DROP_FRAME_THRESHOLD 3      /* 连续丢帧计数阈值，超过则警告 */
#define MAX_DROP_FRAMES      10     /* 最大连续丢帧数，超过则重置配额 */
#define MAX_SEND_ERRORS      5      /* 连续发送错误阈值，超过则断开 */

static const char *s_tag = "CAM_STREAM";

/* ── 客户端状态 ──────────────────────────────────────── */
static httpd_handle_t    s_hd           = NULL;
static int               s_fd           = -1;
static SemaphoreHandle_t s_client_mutex = NULL; /* 保护 s_hd / s_fd */
static SemaphoreHandle_t s_client_ready = NULL; /* 有新客户端时 give */
static SemaphoreHandle_t s_send_quota   = NULL; /* 背压：限制 in-flight 帧数 */
static volatile int      s_send_errors  = 0;    /* 连续发送错误计数 */

/* ── 异步发帧参数 ────────────────────────────────────── */
typedef struct {
    httpd_handle_t hd;
    int            fd;
    uint8_t       *data;
    size_t         len;
} frame_send_arg_t;

/* 在 HTTPD 上下文中执行，发送一帧后释放内存并归还配额 */
static void ws_send_frame_work(void *arg)
{
    frame_send_arg_t *farg = (frame_send_arg_t *)arg;

    httpd_ws_frame_t ws_frame = {
        .type    = HTTPD_WS_TYPE_BINARY,
        .payload = farg->data,
        .len     = farg->len,
        .final   = true,
    };

    esp_err_t ret = httpd_ws_send_frame_async(farg->hd, farg->fd, &ws_frame);
    if (ret != ESP_OK) {
        s_send_errors++;
        ESP_LOGW(s_tag, "Frame send failed (%s), errors=%d",
                 esp_err_to_name(ret), s_send_errors);
        if (s_send_errors >= MAX_SEND_ERRORS) {
            ESP_LOGE(s_tag, "Too many send errors, detaching client");
            camera_stream_detach_client();
        }
    } else {
        s_send_errors = 0;  /* 成功则重置错误计数 */
    }

    heap_caps_free(farg->data);
    free(farg);
    xSemaphoreGive(s_send_quota); /* 归还配额 */
}

/* ── FreeRTOS 任务 ───────────────────────────────────── */
static void camera_stream_task(void *arg)
{
    const hal_camera_ops_t *cam = hal_camera_get();
    if (!cam) {
        ESP_LOGE(s_tag, "Camera HAL not registered, task exiting");
        vTaskDelete(NULL);
        return;
    }

    int drop_count = 0;

    while (true) {
        /* 等待客户端接入 */
        xSemaphoreTake(s_client_ready, portMAX_DELAY);
        ESP_LOGI(s_tag, "Client attached, starting stream");
        drop_count = 0;

        while (true) {
            /* 非阻塞检查发送配额：如果上一帧还在发送，直接丢弃当前帧。
             * 这样可以避免网络拥塞时阻塞采集，保持系统响应性。 */
            if (xSemaphoreTake(s_send_quota, 0) != pdTRUE) {
                /* 配额不可用，丢弃帧 */
                camera_fb_t *fb = cam->fb_get();
                if (fb) {
                    cam->fb_return(fb);
                }
                drop_count++;
                if (drop_count >= DROP_FRAME_THRESHOLD) {
                    ESP_LOGW(s_tag, "Dropped %d frames (network congested)", drop_count);
                }
                /* 连续丢帧过多，强制重置配额（可能是发送任务卡死） */
                if (drop_count >= MAX_DROP_FRAMES) {
                    ESP_LOGW(s_tag, "Too many dropped frames, resetting quota");
                    while (xSemaphoreTake(s_send_quota, 0) == pdTRUE) {}
                    for (int i = 0; i < MAX_INFLIGHT_FRAMES; i++) {
                        xSemaphoreGive(s_send_quota);
                    }
                    drop_count = 0;
                }
                vTaskDelay(pdMS_TO_TICKS(FRAME_INTERVAL_MS));
                continue;
            }
            drop_count = 0;  /* 重置丢帧计数 */

            /* 读取当前客户端（加锁） */
            xSemaphoreTake(s_client_mutex, portMAX_DELAY);
            httpd_handle_t hd = s_hd;
            int            fd = s_fd;
            xSemaphoreGive(s_client_mutex);

            if (!hd || fd < 0) {
                /* 客户端已被 detach，归还配额后退出内层循环 */
                xSemaphoreGive(s_send_quota);
                break;
            }

            /* 采集帧 */
            camera_fb_t *fb = cam->fb_get();
            if (!fb) {
                ESP_LOGW(s_tag, "fb_get returned NULL");
                xSemaphoreGive(s_send_quota);
                vTaskDelay(pdMS_TO_TICKS(FRAME_INTERVAL_MS));
                continue;
            }

            /* 拷贝 JPEG 数据到 PSRAM，以便立刻归还 fb */
            uint8_t *buf = heap_caps_malloc(fb->len,
                                            MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!buf) {
                /* PSRAM 不足时降级到内部 RAM */
                buf = malloc(fb->len);
            }

            if (!buf) {
                ESP_LOGE(s_tag, "No memory for frame copy (len=%zu)", fb->len);
                cam->fb_return(fb);
                xSemaphoreGive(s_send_quota);
                vTaskDelay(pdMS_TO_TICKS(FRAME_INTERVAL_MS));
                continue;
            }

            size_t len = fb->len;
            memcpy(buf, fb->buf, len);
            cam->fb_return(fb); /* 尽快归还，摄像头可继续采集 */

            /* 构造异步发送参数 */
            frame_send_arg_t *farg = malloc(sizeof(frame_send_arg_t));
            if (!farg) {
                heap_caps_free(buf);
                xSemaphoreGive(s_send_quota);
                vTaskDelay(pdMS_TO_TICKS(FRAME_INTERVAL_MS));
                continue;
            }
            farg->hd   = hd;
            farg->fd   = fd;
            farg->data = buf;
            farg->len  = len;

            esp_err_t ret = httpd_queue_work(hd, ws_send_frame_work, farg);
            if (ret != ESP_OK) {
                /* queue_work 失败通常是 HTTPD 内部队列暂时满，属于
                 * 瞬态情况；丢掉本帧继续，由 ws_send_frame_work 的
                 * send_async 失败来判断连接是否真正断开。 */
                ESP_LOGW(s_tag, "httpd_queue_work failed (%s), dropping frame",
                         esp_err_to_name(ret));
                heap_caps_free(buf);
                free(farg);
                xSemaphoreGive(s_send_quota);
                vTaskDelay(pdMS_TO_TICKS(FRAME_INTERVAL_MS * 2));
                continue;
            }

            vTaskDelay(pdMS_TO_TICKS(FRAME_INTERVAL_MS));
        }

        ESP_LOGI(s_tag, "Stream stopped, waiting for new client");
    }
}

/* ── 公开接口 ────────────────────────────────────────── */

void camera_stream_attach_client(httpd_handle_t hd, int fd)
{
    xSemaphoreTake(s_client_mutex, portMAX_DELAY);
    s_hd = hd;
    s_fd = fd;
    xSemaphoreGive(s_client_mutex);

    /* 重置发送配额（防止上次流残留状态） */
    while (xSemaphoreTake(s_send_quota, 0) == pdTRUE) {}
    for (int i = 0; i < MAX_INFLIGHT_FRAMES; i++) {
        xSemaphoreGive(s_send_quota);
    }

    s_send_errors = 0;  /* 重置错误计数 */
    xSemaphoreGive(s_client_ready); /* 唤醒 camera_stream_task */
    ESP_LOGI(s_tag, "Client attached, fd=%d", fd);
}

void camera_stream_detach_client(void)
{
    xSemaphoreTake(s_client_mutex, portMAX_DELAY);
    s_hd = NULL;
    s_fd = -1;
    xSemaphoreGive(s_client_mutex);
    ESP_LOGI(s_tag, "Client detached");
}

esp_err_t camera_stream_start(void)
{
    s_client_mutex = xSemaphoreCreateMutex();
    s_client_ready = xSemaphoreCreateBinary();
    s_send_quota   = xSemaphoreCreateCounting(MAX_INFLIGHT_FRAMES,
                                              MAX_INFLIGHT_FRAMES);

    if (!s_client_mutex || !s_client_ready || !s_send_quota) {
        ESP_LOGE(s_tag, "Failed to create semaphores");
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ret = xTaskCreate(camera_stream_task, "cam_stream",
                                 STREAM_TASK_STACK, NULL,
                                 STREAM_TASK_PRIO, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(s_tag, "Failed to create stream task");
        return ESP_FAIL;
    }

    ESP_LOGI(s_tag, "Stream task started (prio=%d, stack=%d)",
             STREAM_TASK_PRIO, STREAM_TASK_STACK);
    return ESP_OK;
}
