#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

/**
 * @brief 初始化信号量并启动相机流 FreeRTOS 任务
 *
 * 在 WiFi 就绪前调用即可；任务会等待客户端接入后才真正采集帧。
 *
 * @return ESP_OK 成功
 */
esp_err_t camera_stream_start(void);

/**
 * @brief 注册视频 WebSocket 客户端
 *
 * 由 /video WebSocket handler 在握手完成后调用。
 *
 * @param hd  HTTPD 服务器句柄
 * @param fd  客户端 socket fd
 */
void camera_stream_attach_client(httpd_handle_t hd, int fd);

/**
 * @brief 注销当前客户端
 *
 * 由发送失败检测逻辑调用；调用后任务挂起等待新客户端。
 */
void camera_stream_detach_client(void);
