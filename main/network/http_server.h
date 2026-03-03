#pragma once

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"

/**
 * @brief 初始化SPIFFS文件系统
 * @return ESP_OK 成功
 */
esp_err_t init_spiffs(void);

/**
 * @brief 启动HTTP服务器
 * @return 服务器句柄，失败返回NULL
 */
httpd_handle_t start_webserver(void);

/**
 * @brief 停止HTTP服务器
 * @param server 服务器句柄
 * @return ESP_OK 成功
 */
esp_err_t stop_webserver(httpd_handle_t server);

/**
 * @brief Wi-Fi连接事件处理器
 * @param arg 用户参数(服务器句柄指针)
 * @param event_base 事件基
 * @param event_id 事件ID
 * @param event_data 事件数据
 */
void connect_handler(void* arg, esp_event_base_t event_base,
                     int32_t event_id, void* event_data);

/**
 * @brief Wi-Fi断开事件处理器
 * @param arg 用户参数(服务器句柄指针)
 * @param event_base 事件基
 * @param event_id 事件ID
 * @param event_data 事件数据
 */
void disconnect_handler(void* arg, esp_event_base_t event_base,
                        int32_t event_id, void* event_data);
