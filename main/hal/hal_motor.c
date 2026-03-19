#include "hal_motor.h"
#include "esp_log.h"

static const char *s_tag = "HAL_MOTOR";

static const hal_motor_ops_t *s_ops = NULL;

void hal_motor_register(const hal_motor_ops_t *ops)
{
    s_ops = ops;
    ESP_LOGI(s_tag, "Motor HAL registered");
}

const hal_motor_ops_t *hal_motor_get(void)
{
    return s_ops;
}
