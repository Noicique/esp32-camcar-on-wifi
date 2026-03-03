#pragma once

#include "esp_err.h"

/**
 * @brief 初始化电机GPIO引脚
 * @return ESP_OK 成功
 */
esp_err_t motor_gpio_init(void);

/**
 * @brief 初始化电机PWM
 * @return ESP_OK 成功
 */
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
