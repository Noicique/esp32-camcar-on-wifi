#include <esp_log.h>
#include <nvs_flash.h>
#include "esp_wifi.h"
#include "config.h"
#include "hal/hal_motor.h"
#include "hal/hal_camera.h"
#include "motor_control/motor_control.h"
#include "camera/camera_driver.h"
#include "camera/camera_stream.h"
#include "network/wifi_manager.h"
#include "network/http_server.h"
#include "esp_timer.h"

static const char *s_tag = "MAIN";

static const hal_motor_ops_t s_motor_ops = {
    .init       = motor_init,
    .forward    = motor_forward,
    .backward   = motor_backward,
    .turn_left  = motor_turn_left,
    .turn_right = motor_turn_right,
    .stop       = motor_stop,
};

static void delayed_camera_init(void *arg)
{
    ESP_LOGI(s_tag, "Starting delayed camera initialization...");
    
    camera_driver_register();
    esp_err_t ret = hal_camera_get()->init();
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Camera init failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = camera_stream_start();
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Camera stream start failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ESP_LOGI(s_tag, "Camera initialized successfully");
}

void app_main(void)
{
    static httpd_handle_t server = NULL;

    ESP_LOGI(s_tag, "Application starting...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(s_tag, "NVS partition truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ret = init_spiffs();
    ESP_ERROR_CHECK(ret);

    /* mDNS hostname 必须在 wifi_init_sta() 之前注册，
     * 否则 IP_EVENT_STA_GOT_IP 可能在 wifi_mdns_start() 调用前就触发，
     * 导致 s_mdns_on_ip handler 检查到 hostname 为空而跳过 mDNS 初始化。 */
    ret = wifi_mdns_start("esp32-car");
    ESP_ERROR_CHECK(ret);

    ret = wifi_init_sta();
    ESP_ERROR_CHECK(ret);

    /* 注册事件 handler，捕获后续的 IP 获取 / 断开事件 */
    ret = wifi_register_event_handler(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                      &connect_handler, &server);
    ESP_ERROR_CHECK(ret);

    ret = wifi_register_event_handler(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                      &disconnect_handler, &server);
    ESP_ERROR_CHECK(ret);

    /* 主动查询当前状态：若 IP_EVENT_STA_GOT_IP 在 handler 注册前已经触发
     * （快速 AP / 静态IP 场景），事件已丢失，通过状态查询补救。
     * 两条路径互斥（server 指针保护），不会重复启动。 */
    if (wifi_sta_has_ip() && server == NULL) {
        ESP_LOGI(s_tag, "IP already available before handler registered, starting server now");
        esp_wifi_set_ps(WIFI_PS_NONE);
        server = start_webserver();
    }

    hal_motor_register(&s_motor_ops);
    ret = s_motor_ops.init();
    ESP_ERROR_CHECK(ret);

    /* 延迟初始化摄像头：等待网络稳定后再启动摄像头，
     * 避免 DMA 和 WiFi 总线冲突导致网络响应缓慢。
     * 延迟 3 秒让 HTTP 服务器先正常响应请求。 */
    esp_timer_handle_t cam_timer = NULL;
    esp_timer_create_args_t timer_args = {
        .callback = delayed_camera_init,
        .name = "cam_init_timer",
    };
    ret = esp_timer_create(&timer_args, &cam_timer);
    if (ret == ESP_OK) {
        esp_timer_start_once(cam_timer, 3000000);  /* 3 秒后初始化摄像头 */
        ESP_LOGI(s_tag, "Camera init scheduled in 3 seconds");
    } else {
        ESP_LOGW(s_tag, "Timer create failed, init camera now");
        delayed_camera_init(NULL);
    }

    ESP_LOGI(s_tag, "System initialized successfully");
}
