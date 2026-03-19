#pragma once

#include "esp_err.h"

/**
 * @brief 电机 HAL 操作接口
 *
 * 平台实现（motor_control.c）填充此结构体，
 * 通过 hal_motor_register() 注册后，上层通过
 * hal_motor_get() 取得指针并调用。
 *
 * FreeRTOS 说明：command_task 读取命令队列后，
 * 通过此接口驱动电机，不直接依赖平台代码。
 */
typedef struct {
    esp_err_t (*init)(void);
    void (*forward)(void);
    void (*backward)(void);
    void (*turn_left)(void);
    void (*turn_right)(void);
    void (*stop)(void);
} hal_motor_ops_t;

/**
 * @brief 注册电机 HAL 实现（在 app_main 中调用，早于任何驱动操作）
 */
void hal_motor_register(const hal_motor_ops_t *ops);

/**
 * @brief 获取已注册的电机 HAL 指针（未注册时返回 NULL）
 */
const hal_motor_ops_t *hal_motor_get(void);
