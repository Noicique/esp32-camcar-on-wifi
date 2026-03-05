#include "camera_driver.h"
#include "../config.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

static const char *s_tag = "CAMERA";

static bool s_camera_initialized = false;

esp_err_t camera_init(void)
{
    if (s_camera_initialized) {
        ESP_LOGW(s_tag, "Camera already initialized");
        return ESP_OK;
    }

    ESP_LOGI(s_tag, "Initializing camera...");
    ESP_LOGI(s_tag, "Free heap: %u, Free PSRAM: %u", 
             (unsigned int)esp_get_free_heap_size(), 
             (unsigned int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));

    camera_config_t config = {
        .pin_pwdn = CAM_PIN_PWDN,
        .pin_reset = CAM_PIN_RESET,
        .pin_xclk = CAM_PIN_XCLK,
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
        .pin_href = CAM_PIN_HREF,
        .pin_pclk = CAM_PIN_PCLK,
        
        .xclk_freq_hz = CAM_XCLK_FREQ,
        .ledc_timer = LEDC_TIMER_0,
        .ledc_channel = LEDC_CHANNEL_0,
        
        .pixel_format = PIXFORMAT_JPEG,
        .frame_size = CAM_FRAME_SIZE,
        .jpeg_quality = CAM_JPEG_QUALITY,
        .fb_count = 2,
        .fb_location = CAMERA_FB_IN_PSRAM,
        .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    };

    esp_err_t ret = esp_camera_init(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Camera init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor != NULL) {
        ESP_LOGI(s_tag, "Camera sensor detected: %s", sensor->id.PID == OV2640_PID ? "OV2640" : "Unknown");
        
        sensor->set_framesize(sensor, CAM_FRAME_SIZE);
        sensor->set_quality(sensor, CAM_JPEG_QUALITY);
        sensor->set_brightness(sensor, 0);
        sensor->set_contrast(sensor, 0);
        sensor->set_saturation(sensor, 0);
    }

    s_camera_initialized = true;
    ESP_LOGI(s_tag, "Camera initialized successfully");
    ESP_LOGI(s_tag, "Free heap after init: %u, Free PSRAM: %u", 
             (unsigned int)esp_get_free_heap_size(), 
             (unsigned int)heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    
    return ESP_OK;
}

camera_fb_t* camera_fb_get(void)
{
    if (!s_camera_initialized) {
        ESP_LOGE(s_tag, "Camera not initialized");
        return NULL;
    }
    
    camera_fb_t *fb = esp_camera_fb_get();
    if (fb == NULL) {
        ESP_LOGE(s_tag, "Failed to get camera frame buffer");
        return NULL;
    }
    
    return fb;
}

void camera_fb_return(camera_fb_t *fb)
{
    if (fb != NULL) {
        esp_camera_fb_return(fb);
    }
}
