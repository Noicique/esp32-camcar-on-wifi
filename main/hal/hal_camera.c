#include "hal_camera.h"
#include "esp_log.h"

static const char *s_tag = "HAL_CAMERA";

static const hal_camera_ops_t *s_ops = NULL;

void hal_camera_register(const hal_camera_ops_t *ops)
{
    s_ops = ops;
    ESP_LOGI(s_tag, "Camera HAL registered");
}

const hal_camera_ops_t *hal_camera_get(void)
{
    return s_ops;
}
