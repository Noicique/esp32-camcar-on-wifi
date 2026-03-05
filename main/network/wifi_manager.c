#include "wifi_manager.h"
#include "wifi_config.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/ip_addr.h"

static const char *s_tag = "WIFI";
static esp_netif_t *s_sta_netif = NULL;

#if USE_STATIC_IP
static esp_err_t configure_static_ip(void)
{
    esp_netif_ip_info_t ip_info;
    
    ip_info.ip.addr = ipaddr_addr(STATIC_IP_ADDR);
    ip_info.gw.addr = ipaddr_addr(STATIC_GATEWAY);
    ip_info.netmask.addr = ipaddr_addr(STATIC_NETMASK);
    
    if (s_sta_netif == NULL) {
        ESP_LOGE(s_tag, "STA netif is NULL");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t ret = esp_netif_dhcpc_stop(s_sta_netif);
    if (ret != ESP_OK && ret != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
        ESP_LOGE(s_tag, "DHCP stop failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_netif_set_ip_info(s_sta_netif, &ip_info);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Set IP info failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    esp_netif_dns_info_t dns_info;
    dns_info.ip.u_addr.ip4.addr = ipaddr_addr(STATIC_DNS_PRIMARY);
    dns_info.ip.type = IPADDR_TYPE_V4;
    ret = esp_netif_set_dns_info(s_sta_netif, ESP_NETIF_DNS_MAIN, &dns_info);
    if (ret != ESP_OK) {
        ESP_LOGW(s_tag, "Set primary DNS failed: %s", esp_err_to_name(ret));
    }
    
    dns_info.ip.u_addr.ip4.addr = ipaddr_addr(STATIC_DNS_SECONDARY);
    ret = esp_netif_set_dns_info(s_sta_netif, ESP_NETIF_DNS_BACKUP, &dns_info);
    if (ret != ESP_OK) {
        ESP_LOGW(s_tag, "Set backup DNS failed: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(s_tag, "Static IP configured: %s", STATIC_IP_ADDR);
    return ESP_OK;
}
#endif

esp_err_t wifi_init_netif(void)
{
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Netif init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(s_tag, "Event loop create failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (s_sta_netif == NULL) {
        ESP_LOGE(s_tag, "Create STA netif failed");
        return ESP_FAIL;
    }
    
    ESP_LOGI(s_tag, "Netif initialized");
    return ESP_OK;
}

esp_err_t wifi_init_sta(void)
{
    esp_err_t ret;
    
#if USE_STATIC_IP
    ret = configure_static_ip();
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Static IP config failed: %s", esp_err_to_name(ret));
        return ret;
    }
#endif
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "WiFi init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTHMODE,
        },
    };
    
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Set WiFi mode failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Set WiFi config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "WiFi start failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "WiFi connect failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(s_tag, "WiFi init STA finished, connecting to SSID: %s", WIFI_SSID);
#if USE_STATIC_IP
    ESP_LOGI(s_tag, "Using static IP: %s", STATIC_IP_ADDR);
#endif
    return ESP_OK;
}

esp_err_t wifi_register_event_handler(esp_event_base_t event_base, 
                                       int32_t event_id,
                                       esp_event_handler_t event_handler,
                                       void *arg)
{
    esp_err_t ret = esp_event_handler_register(event_base, event_id, event_handler, arg);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Event handler register failed: %s", esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}
