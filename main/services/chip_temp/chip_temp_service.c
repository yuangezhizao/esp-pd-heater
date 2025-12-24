#include "app_services.h"

#include "app_events.h"
#include "app_state.h"
#include "driver/temperature_sensor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "chip_temp"

static void chip_temp_task(void *pvParameter) {
    (void)pvParameter;

    ESP_LOGI(TAG, "Install temperature sensor");
    temperature_sensor_handle_t temp_sensor = NULL;
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 100);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));

    ESP_LOGI(TAG, "Enable temperature sensor");
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));

    while (1) {
        float chip_temp = 0;
        ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_sensor, &chip_temp));
        app_state_lock();
        g_state.temp.chip = chip_temp;
        app_state_unlock();

        if (chip_temp > 85) {
            app_event_t ev = {.type = APP_EVENT_FORCE_STOP_HEATING, .reason = APP_STOP_REASON_OVERTEMP};
            (void)app_events_post(&ev, 0);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_service_chip_temp_start(void) {
    xTaskCreate(chip_temp_task, "chip_temp", 4096, NULL, 10, NULL);
}

