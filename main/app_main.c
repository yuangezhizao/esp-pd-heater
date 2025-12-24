#include "app_buzzer.h"
#include "app_controller.h"
#include "app_services.h"
#include "app_state.h"
#include "app_ui.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#define TAG "esp-pd-heater"

static i2c_bus_handle_t s_i2c_bus = NULL;

void app_main(void) {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Initialize state
    app_state_init();
    app_state_load();

    // Init I2C bus (shared)
    ESP_ERROR_CHECK(bsp_i2c_init());
    s_i2c_bus = bsp_i2c_get_bus();
    if (s_i2c_bus == NULL) {
        ESP_LOGE(TAG, "bsp_i2c_get_bus returned NULL");
        abort();
    }

    // One-shot boot beep
    xTaskCreate(buzzer_start_task, "buzzer_start_task", 2048, NULL, 10, NULL);

    // Display + LVGL
    bsp_display_start();
    app_lvgl_display();

    // Controller (events + side-effects)
    app_controller_start();

    // Services
    app_service_backlight_start();
    app_service_temp_control_start();
    app_service_power_meter_start(s_i2c_bus);
    app_service_heater_pwm_start();
    app_service_usb_pd_start(s_i2c_bus);
    app_service_chip_temp_start();
    app_service_motion_start(s_i2c_bus);

    // Init UI widget values from state
    lvgl_widgets_values_init();

    // Periodically save NVS parameters
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        app_state_save();
    }
}

