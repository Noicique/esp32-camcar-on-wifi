#pragma once

#include "esp_err.h"

/**
 * @brief 小车命令枚举
 *
 * 网络层（http_server）将收到的字符串解析为此枚举，
 * 再交给 command_dispatch() 分发，不直接调用任何硬件接口。
 */
typedef enum {
    CAR_CMD_FORWARD,
    CAR_CMD_BACKWARD,
    CAR_CMD_TURN_LEFT,
    CAR_CMD_TURN_RIGHT,
    CAR_CMD_STOP,
    CAR_CMD_UNKNOWN,
} car_cmd_t;

/**
 * @brief 将 WebSocket 文本命令解析为 car_cmd_t
 * @param str 以 '\0' 结尾的命令字符串
 * @return 对应的命令枚举，无法识别时返回 CAR_CMD_UNKNOWN
 */
car_cmd_t command_parse(const char *str);

/**
 * @brief 将命令分发到已注册的 HAL 执行
 *
 * 当前实现：同步直接调用 HAL 函数。
 *
 * FreeRTOS 升级路径：
 *   将此函数替换为 xQueueSend()，
 *   由独立的 command_task 读取队列后执行 HAL，
 *   从而使网络任务不阻塞在硬件操作上。
 */
void command_dispatch(car_cmd_t cmd);

/* ── FreeRTOS 预留接口（暂未实现） ──────────────────────────────────────
 * esp_err_t command_queue_init(void);
 * esp_err_t command_send(car_cmd_t cmd);   // xQueueSend
 * void      command_task(void *arg);       // FreeRTOS 任务入口
 * ──────────────────────────────────────────────────────────────────── */
