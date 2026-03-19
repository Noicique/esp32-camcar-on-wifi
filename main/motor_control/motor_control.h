#pragma once

#include "esp_err.h"

/**
 * @brief 初始化电机（GPIO + PWM），作为 hal_motor_ops_t.init 的实现
 * @return ESP_OK 成功
 */
esp_err_t motor_init(void);

/* 以下两个函数保留供内部调用，通常不直接使用 */
esp_err_t motor_gpio_init(void);
esp_err_t motor_pwm_init(void);

/**
 * @brief 电机前进
 */
void motor_forward(void);

/**
 * @brief 电机后退
 */
void motor_backward(void);

/**
 * @brief 电机左转
 */
void motor_turn_left(void);

/**
 * @brief 电机右转
 */
void motor_turn_right(void);

/**
 * @brief 电机停止
 */
void motor_stop(void);
