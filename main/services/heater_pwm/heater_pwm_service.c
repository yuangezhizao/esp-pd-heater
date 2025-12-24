#include "app_services.h"

#include "app_state.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "heater_pwm"

#define HEATER_GPIO GPIO_NUM_3

#define LEDC_TIMER LEDC_TIMER_1
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_3
#define LEDC_DUTY_RES LEDC_TIMER_10_BIT
#define LEDC_FREQUENCY (30 * 1000)

static void heater_pwm_task(void *pvParameter) {
    (void)pvParameter;

    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .gpio_num = HEATER_GPIO,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    uint32_t last_duty = 0;

    while (1) {
        app_state_t st = {0};
        app_state_snapshot(&st);

        uint32_t duty = st.heating.on ? st.heating.pwm_current_duty : 0;
        if (duty != last_duty) {
            last_duty = duty;
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void app_service_heater_pwm_start(void) {
    xTaskCreate(heater_pwm_task, "heater_pwm", 4096, NULL, 2, NULL);
}

