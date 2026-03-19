#pragma once

#include "esp_err.h"
#include "esp_camera.h"

/**
 * @brief 相机 HAL 操作接口
 *
 * 用于视频流任务（camera_stream_task）与具体相机驱动解耦。
 * 视频流计划以独立 FreeRTOS 高优先级任务实现，
 * 通过此接口获取/归还帧缓冲，不直接依赖 esp_camera API。
 *
 * 当前状态：接口已定义，实现待补充。
 */
typedef struct {
    esp_err_t    (*init)(void);
    camera_fb_t *(*fb_get)(void);
    void         (*fb_return)(camera_fb_t *fb);
} hal_camera_ops_t;

/**
 * @brief 注册相机 HAL 实现
 */
void hal_camera_register(const hal_camera_ops_t *ops);

/**
 * @brief 获取已注册的相机 HAL 指针（未注册时返回 NULL）
 */
const hal_camera_ops_t *hal_camera_get(void);
