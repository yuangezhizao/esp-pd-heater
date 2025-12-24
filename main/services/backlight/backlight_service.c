#include "app_services.h"

#include "app_state.h"
#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void backlight_start_task(void *pvParameter) {
    (void)pvParameter;
    vTaskDelay(pdMS_TO_TICKS(200));
    app_state_t st = {0};
    app_state_snapshot(&st);
    bsp_display_brightness_set(st.config.brightness, 2000);
    vTaskDelete(NULL);
}

void app_service_backlight_start(void) {
    xTaskCreate(backlight_start_task, "backlight_start", 2048, NULL, 3, NULL);
}

