#pragma once

/**
 * @brief 将 esp_camera 驱动注册到相机 HAL
 *
 * 在 app_main 中调用一次，之后上层通过 hal_camera_get() 操作相机，
 * 不直接依赖 esp_camera API。
 */
void camera_driver_register(void);
