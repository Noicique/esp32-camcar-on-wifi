#include "wifi_manager.h"
#include "wifi_config.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "lwip/ip_addr.h"
#include "mdns.h"
#include "esp_timer.h"

static const char *s_tag = "WIFI";
static esp_netif_t *s_sta_netif = NULL;
static char s_mdns_hostname[64] = {0};
static esp_timer_handle_t s_reconnect_timer = NULL;

static void s_reconnect_cb(void *arg)
{
    ESP_LOGI(s_tag, "Reconnecting...");
    esp_wifi_connect();
}

/* 内部 WiFi 连接管理：STA_START 触发首次连接，STA_DISCONNECTED 延迟 1s 后重连。
 * 延迟避免立即重试导致 AP 触发限速策略（802.11 status 39）。 */
static void s_wifi_sta_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (id == WIFI_EVENT_STA_START) {
        ESP_LOGI(s_tag, "STA started, connecting...");
        esp_wifi_connect();
    } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(s_tag, "Disconnected, retrying in 1s...");
        esp_timer_start_once(s_reconnect_timer, 1000 * 1000); /* 1 秒 */
    }
}

static void s_mdns_on_ip(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (s_mdns_hostname[0] == '\0') return;
    esp_err_t ret = mdns_init();
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "mDNS init failed: %s", esp_err_to_name(ret));
        return;
    }
    mdns_hostname_set(s_mdns_hostname);
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    ESP_LOGI(s_tag, "mDNS started: http://%s.local", s_mdns_hostname);
}

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

esp_err_t wifi_init_sta(void)
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
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
#if USE_STATIC_IP
    ret = configure_static_ip();
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Static IP config failed: %s", esp_err_to_name(ret));
        return ret;
    }
#endif    
    
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "WiFi init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_timer_create_args_t timer_args = {
        .callback = s_reconnect_cb,
        .name     = "wifi_reconnect",
    };
    ret = esp_timer_create(&timer_args, &s_reconnect_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "Reconnect timer create failed: %s", esp_err_to_name(ret));
        return ret;
    }

    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, s_wifi_sta_handler, NULL);
    esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, s_wifi_sta_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, s_mdns_on_ip, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTHMODE,
#if WIFI_BSSID_LOCK
            .bssid_set = true,
#endif
        },
    };

#if WIFI_BSSID_LOCK
    sscanf(WIFI_BSSID, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &wifi_config.sta.bssid[0], &wifi_config.sta.bssid[1],
           &wifi_config.sta.bssid[2], &wifi_config.sta.bssid[3],
           &wifi_config.sta.bssid[4], &wifi_config.sta.bssid[5]);
    ESP_LOGI(s_tag, "BSSID lock enabled: %s", WIFI_BSSID);
#endif
    
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
    /* esp_wifi_connect() 由 WIFI_EVENT_STA_START handler 内部触发，无需在此调用 */

    ESP_LOGI(s_tag, "WiFi init STA finished, connecting to SSID: %s", WIFI_SSID);
#if USE_STATIC_IP
    ESP_LOGI(s_tag, "Using static IP: %s", STATIC_IP_ADDR);
#endif
    return ESP_OK;
}

bool wifi_sta_has_ip(void)
{
    if (s_sta_netif == NULL) {
        return false;
    }
    esp_netif_ip_info_t ip_info;
    if (esp_netif_get_ip_info(s_sta_netif, &ip_info) != ESP_OK) {
        return false;
    }
    return ip_info.ip.addr != 0;
}

esp_err_t wifi_mdns_start(const char *hostname)
{
    strncpy(s_mdns_hostname, hostname, sizeof(s_mdns_hostname) - 1);
    s_mdns_hostname[sizeof(s_mdns_hostname) - 1] = '\0';
    ESP_LOGI(s_tag, "mDNS hostname registered: %s (starts after IP obtained)", hostname);
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
