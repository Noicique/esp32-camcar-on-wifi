#include <esp_log.h>
#include <nvs_flash.h>
#include "config.h"
#include "hal/hal_motor.h"
#include "motor_control/motor_control.h"
#include "network/wifi_manager.h"
#include "network/http_server.h"

static const char *s_tag = "MAIN";

/* 将 ESP32 平台实现注册到电机 HAL */
static const hal_motor_ops_t s_motor_ops = {
    .init       = motor_init,
    .forward    = motor_forward,
    .backward   = motor_backward,
    .turn_left  = motor_turn_left,
    .turn_right = motor_turn_right,
    .stop       = motor_stop,
};

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

    ret = wifi_init_sta();
    ESP_ERROR_CHECK(ret);

    /* 注册 HAL 实现，上层（command_handler）通过 hal_motor_get() 调用 */
    hal_motor_register(&s_motor_ops);
    ret = s_motor_ops.init();
    ESP_ERROR_CHECK(ret);

    ret = wifi_register_event_handler(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                      &connect_handler, &server);
    ESP_ERROR_CHECK(ret);

    ret = wifi_register_event_handler(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
                                      &disconnect_handler, &server);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(s_tag, "System initialized successfully");

    /* FreeRTOS 升级路径（预留注释）：
     * xTaskCreate(command_task, "cmd",  4096, NULL, 5, NULL);
     * xTaskCreate(camera_stream_task, "cam", 8192, NULL, 6, NULL);
     */
}
