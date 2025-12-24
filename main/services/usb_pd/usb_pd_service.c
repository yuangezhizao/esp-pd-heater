#include "app_services.h"

#include "app_usb_pd.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "usb_pd_service"

static i2c_bus_handle_t s_i2c_bus = NULL;

static void usb_pd_task(void *pvParameter) {
    (void)pvParameter;

    app_pd_init(s_i2c_bus);

    size_t count = 0;
    while (1) {
        app_pd_update();

        if (count <= 3) {
            app_pd_request_max_voltage();
            count++;
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_service_usb_pd_start(i2c_bus_handle_t i2c_bus) {
    s_i2c_bus = i2c_bus;
    xTaskCreate(usb_pd_task, "usb_pd", 4096, NULL, 10, NULL);
}

