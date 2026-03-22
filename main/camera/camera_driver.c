#include "camera_driver.h"
#include "config.h"
#include "hal/hal_camera.h"
#include "esp_camera.h"
#include "esp_log.h"
#include "driver/ledc.h"

static const char *s_tag = "CAM_DRIVER";

static esp_err_t cam_init(void)
{
    camera_config_t config = {
        .pin_pwdn     = CAM_PIN_PWDN,
        .pin_reset    = CAM_PIN_RESET,
        .pin_xclk     = CAM_PIN_XCLK,
        .pin_sscb_sda = CAM_PIN_SIOD,
        .pin_sscb_scl = CAM_PIN_SIOC,

        .pin_d7 = CAM_PIN_Y9,
        .pin_d6 = CAM_PIN_Y8,
        .pin_d5 = CAM_PIN_Y7,
        .pin_d4 = CAM_PIN_Y6,
        .pin_d3 = CAM_PIN_Y5,
        .pin_d2 = CAM_PIN_Y4,
        .pin_d1 = CAM_PIN_Y3,
        .pin_d0 = CAM_PIN_Y2,

        .pin_vsync = CAM_PIN_VSYNC,
        .pin_href  = CAM_PIN_HREF,
        .pin_pclk  = CAM_PIN_PCLK,

        .xclk_freq_hz = CAM_XCLK_FREQ,

        /* 使用与电机 PWM 不同的 LEDC timer/channel，避免冲突
         * 电机使用 LEDC_TIMER_0 / CHANNEL_0~1 */
        .ledc_timer   = LEDC_TIMER_1,
        .ledc_channel = LEDC_CHANNEL_2,

        .pixel_format = PIXFORMAT_JPEG,
        .frame_size   = FRAMESIZE_QQVGA,  /* 160×120，帧 2-5KB，适配 lwIP 默认 TCP 缓冲 */
        .jpeg_quality = 31,               /* 0-63；越大文件越小，带宽优先 */
        .fb_count     = 1,                /* 单缓冲，减少 PSRAM 占用和 DMA 冲突 */
        .fb_location  = CAMERA_FB_IN_PSRAM,
        .grab_mode    = CAMERA_GRAB_LATEST, /* 缓冲区满后 DMA 停止，不干扰 WiFi */
    };

    esp_err_t ret = esp_camera_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Camera init failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(s_tag, "Camera initialized (QVGA JPEG q=%d)", config.jpeg_quality);
    }
    return ret;
}

static camera_fb_t *cam_fb_get(void)
{
    return esp_camera_fb_get();
}

static void cam_fb_return(camera_fb_t *fb)
{
    esp_camera_fb_return(fb);
}

static const hal_camera_ops_t s_cam_ops = {
    .init      = cam_init,
    .fb_get    = cam_fb_get,
    .fb_return = cam_fb_return,
};

void camera_driver_register(void)
{
    hal_camera_register(&s_cam_ops);
}
