#include <math.h>

#include "app_events.h"
#include "app_state.h"
#include "app_ui.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "ui.h"

#define TAG "ui_data_record"

static void data_record_task(void *pvParameter) {
    (void)pvParameter;

    bool data_record_is_running = false;
    app_state_t st = {0};

    while (1) {
        app_state_snapshot(&st);
        // Detect whether we should reset and restart recording
        if (st.system.data_record_restart) {
            app_state_lock();
            g_state.system.data_record_restart = false;
            app_state_unlock();
            data_record_is_running = true;

            bsp_display_lock(0);
            lv_chart_set_all_value(ui_Chart1, ui_Chart1_series_1, LV_CHART_POINT_NONE);
            lv_chart_set_all_value(ui_Chart1, ui_Chart1_series_2, LV_CHART_POINT_NONE);
            bsp_display_unlock();

            ESP_LOGI(TAG, "Chart data reset and restarted");
        }

        if (st.heating.on && data_record_is_running) {
            for (size_t i = 0; i < 121; i++) {
                app_state_snapshot(&st);
                bsp_display_lock(0);
                lv_chart_set_value_by_id(ui_Chart1, ui_Chart1_series_1, i, (int32_t)lroundf(st.temp.pt1000));
                lv_chart_set_value_by_id(ui_Chart1, ui_Chart1_series_2, i, (int32_t)lroundf(st.power.current * 100));
                bsp_display_unlock();

                vTaskDelay(pdMS_TO_TICKS(1000));
                app_state_snapshot(&st);
                if (!st.heating.on || st.system.data_record_restart) break;
            }
            data_record_is_running = false;
            ESP_LOGI(TAG, "Save temp data done");
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_ui_data_record_start(void) {
    xTaskCreate(data_record_task, "ui_data_record", 4096, NULL, 8, NULL);
}
