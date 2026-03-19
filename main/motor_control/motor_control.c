#include "motor_control.h"
#include "config.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *s_tag = "MOTOR";

esp_err_t motor_init(void)
{
    esp_err_t ret = motor_gpio_init();
    if (ret != ESP_OK) return ret;
    return motor_pwm_init();
}

esp_err_t motor_gpio_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MOTOR_AIN1) | 
                        (1ULL << MOTOR_AIN2) | 
                        (1ULL << MOTOR_BIN1) | 
                        (1ULL << MOTOR_BIN2) | 
                        (1ULL << MOTOR_STBY),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "GPIO config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    gpio_set_level(MOTOR_STBY, 1);
    gpio_set_level(MOTOR_AIN1, 0);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN1, 0);
    gpio_set_level(MOTOR_BIN2, 0);
    
    ESP_LOGI(s_tag, "Motor GPIO initialized");
    return ESP_OK;
}

esp_err_t motor_pwm_init(void)
{
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = PWM_RESOLUTION,
        .freq_hz = PWM_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "PWM timer config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ledc_channel_config_t pwma_conf = {
        .channel = LEDC_CHANNEL_0,
        .duty = 0,
        .gpio_num = MOTOR_PWMA,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0
    };
    ret = ledc_channel_config(&pwma_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "PWMA channel config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ledc_channel_config_t pwmb_conf = {
        .channel = LEDC_CHANNEL_1,
        .duty = 0,
        .gpio_num = MOTOR_PWMB,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0
    };
    ret = ledc_channel_config(&pwmb_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(s_tag, "PWMB channel config failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(s_tag, "Motor PWM initialized");
    return ESP_OK;
}

void motor_forward(void)
{
    ESP_LOGI(s_tag, "Motor forward");
    gpio_set_level(MOTOR_AIN1, 1);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN1, 1);
    gpio_set_level(MOTOR_BIN2, 0);
    gpio_set_level(MOTOR_STBY, 1);
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, PWM_DUTY_CYCLE);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, PWM_DUTY_CYCLE);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

void motor_backward(void)
{
    ESP_LOGI(s_tag, "Motor backward");
    gpio_set_level(MOTOR_AIN1, 0);
    gpio_set_level(MOTOR_AIN2, 1);
    gpio_set_level(MOTOR_BIN1, 0);
    gpio_set_level(MOTOR_BIN2, 1);
    gpio_set_level(MOTOR_STBY, 1);
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, PWM_DUTY_CYCLE);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, PWM_DUTY_CYCLE);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

void motor_turn_left(void)
{
    ESP_LOGI(s_tag, "Motor turn left");
    gpio_set_level(MOTOR_AIN1, 0);
    gpio_set_level(MOTOR_AIN2, 1);
    gpio_set_level(MOTOR_BIN1, 1);
    gpio_set_level(MOTOR_BIN2, 0);
    gpio_set_level(MOTOR_STBY, 1);
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, PWM_DUTY_CYCLE);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, PWM_DUTY_CYCLE);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

void motor_turn_right(void)
{
    ESP_LOGI(s_tag, "Motor turn right");
    gpio_set_level(MOTOR_AIN1, 1);
    gpio_set_level(MOTOR_AIN2, 0);
    gpio_set_level(MOTOR_BIN1, 0);
    gpio_set_level(MOTOR_BIN2, 1);
    gpio_set_level(MOTOR_STBY, 1);
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, PWM_DUTY_CYCLE);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, PWM_DUTY_CYCLE);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}

void motor_stop(void)
{
    ESP_LOGI(s_tag, "Motor stop");
    gpio_set_level(MOTOR_AIN1, 1);
    gpio_set_level(MOTOR_AIN2, 1);
    gpio_set_level(MOTOR_BIN1, 1);
    gpio_set_level(MOTOR_BIN2, 1);
    gpio_set_level(MOTOR_STBY, 1);
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
}
