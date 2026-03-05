#pragma once

#include "esp_err.h"
#include "esp_camera.h"

/**
 * @brief 初始化摄像头硬件
 * @return ESP_OK 成功，其他值失败
 * @note 需要PSRAM支持，帧缓冲将分配在PSRAM中
 */
esp_err_t camera_init(void);

/**
 * @brief 获取摄像头帧缓冲
 * @return 帧缓冲指针，失败返回NULL
 * @note 使用完毕后必须调用camera_fb_return()释放
 */
camera_fb_t* camera_fb_get(void);

/**
 * @brief 释放摄像头帧缓冲
 * @param fb 帧缓冲指针
 */
void camera_fb_return(camera_fb_t *fb);
