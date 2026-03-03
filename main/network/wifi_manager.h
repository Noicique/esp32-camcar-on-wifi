#pragma once

#include "esp_err.h"
#include "esp_event.h"

/**
 * @brief 初始化Wi-Fi Station模式
 * @return ESP_OK 成功
 */
esp_err_t wifi_init_sta(void);

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
