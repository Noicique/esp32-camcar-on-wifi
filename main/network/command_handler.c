#include "command_handler.h"
#include "hal/hal_motor.h"
#include "esp_log.h"
#include <string.h>

static const char *s_tag = "CMD";

car_cmd_t command_parse(const char *str)
{
    if (strcmp(str, "forward_start")  == 0) return CAR_CMD_FORWARD;
    if (strcmp(str, "backward_start") == 0) return CAR_CMD_BACKWARD;
    if (strcmp(str, "left_start")     == 0) return CAR_CMD_TURN_LEFT;
    if (strcmp(str, "turn_left")      == 0) return CAR_CMD_TURN_LEFT;
    if (strcmp(str, "right_start")    == 0) return CAR_CMD_TURN_RIGHT;
    if (strcmp(str, "turn_right")     == 0) return CAR_CMD_TURN_RIGHT;
    if (strcmp(str, "stop")           == 0) return CAR_CMD_STOP;
    return CAR_CMD_UNKNOWN;
}

void command_dispatch(car_cmd_t cmd)
{
    const hal_motor_ops_t *motor = hal_motor_get();
    if (motor == NULL) {
        ESP_LOGE(s_tag, "Motor HAL not registered");
        return;
    }

    switch (cmd) {
        case CAR_CMD_FORWARD:    if (motor->forward)    motor->forward();    break;
        case CAR_CMD_BACKWARD:   if (motor->backward)   motor->backward();   break;
        case CAR_CMD_TURN_LEFT:  if (motor->turn_left)  motor->turn_left();  break;
        case CAR_CMD_TURN_RIGHT: if (motor->turn_right) motor->turn_right(); break;
        case CAR_CMD_STOP:       if (motor->stop)       motor->stop();       break;
        default:
            ESP_LOGW(s_tag, "Unknown command: %d", cmd);
            break;
    }
}
