#include <esp_log.h>
#include <nvs_flash.h>
#include "config.h"
#include "motor_control/motor_control.h"
#include "camera/camera_driver.h"
#include "network/wifi_manager.h"
#include "network/http_server.h"

static const char *s_tag = "MAIN";

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
    
    ret = wifi_init_netif();
    ESP_ERROR_CHECK(ret);
    
    ret = wifi_register_event_handler(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                       &connect_handler, &server);
    ESP_ERROR_CHECK(ret);
    
    ret = wifi_register_event_handler(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, 
                                       &disconnect_handler, &server);
    ESP_ERROR_CHECK(ret);
    
    ret = wifi_init_sta();
    ESP_ERROR_CHECK(ret);
    
    ret = camera_init();
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Camera init failed: %s", esp_err_to_name(ret));
    }
    
    ret = motor_gpio_init();
    ESP_ERROR_CHECK(ret);
    
    ret = motor_pwm_init();
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(s_tag, "System initialized successfully");
}
