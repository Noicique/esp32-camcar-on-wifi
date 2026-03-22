#pragma once

#include "esp_err.h"
#include "esp_event.h"
#include <stdbool.h>

/**
 * @brief 初始化Wi-Fi Station模式并发起连接
 * @return ESP_OK 成功
 */
esp_err_t wifi_init_sta(void);

/**
 * @brief 查询STA当前是否已获得IP地址
 *
 * 用于在事件handler注册之后主动轮询状态，
 * 防止 IP_EVENT_STA_GOT_IP 在 handler 注册前已触发而被错过。
 *
 * @return true 已有IP，false 尚未连接或未获取IP
 */
bool wifi_sta_has_ip(void);

/**
 * @brief 启动 mDNS 服务，使设备可通过 <hostname>.local 访问
 *
 * 必须在 wifi_init_sta() 之前调用，否则 IP_EVENT_STA_GOT_IP 可能在
 * hostname 写入前触发，导致 mDNS 不启动。
 *
 * @param hostname 主机名，不含 ".local" 后缀（如 "esp32-car"）
 * @return ESP_OK 成功
 */
esp_err_t wifi_mdns_start(const char *hostname);

/**
 * @brief 注册Wi-Fi事件处理器
 * @param event_base 事件基
 * @param event_id 事件ID
 * @param event_handler 事件处理函数
 * @param arg 用户参数
 * @return ESP_OK 成功
 */
esp_err_t wifi_register_event_handler(esp_event_base_t event_base, 
                                       int32_t event_id,
                                       esp_event_handler_t event_handler,
                                       void *arg);
