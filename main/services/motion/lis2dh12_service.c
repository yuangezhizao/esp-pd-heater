#include "app_services.h"

#include <math.h>

#include "app_events.h"
#include "app_state.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lis2dh12.h"

#define TAG "lis2dh12_service"

static i2c_bus_handle_t s_i2c_bus = NULL;

static void lis2dh12_task(void *pvParameter) {
    (void)pvParameter;

    lis2dh12_handle_t lis2dh12 = lis2dh12_create(s_i2c_bus, LIS2DH12_I2C_ADDRESS);

    uint8_t deviceid;
    lis2dh12_get_deviceid(lis2dh12, &deviceid);
    ESP_LOGI(TAG, "LIS2DH12 device id is: %02x", deviceid);

    if (deviceid != 0x33) {
        ESP_LOGW(TAG, "LIS2DH12 device id is not 0x33, please check the connection");
        lis2dh12_delete(&lis2dh12);
        vTaskDelete(NULL);
    }

    lis2dh12_config_t cfg;
    lis2dh12_get_config(lis2dh12, &cfg);
    cfg.temp_enable = LIS2DH12_TEMP_DISABLE;
    cfg.odr = LIS2DH12_ODR_25HZ;
    cfg.opt_mode = LIS2DH12_OPT_NORMAL;
    cfg.z_enable = LIS2DH12_ENABLE;
    cfg.y_enable = LIS2DH12_ENABLE;
    cfg.x_enable = LIS2DH12_ENABLE;
    cfg.bdu_status = LIS2DH12_DISABLE;
    cfg.fs = LIS2DH12_FS_2G;
    lis2dh12_set_config(lis2dh12, &cfg);

    bool is_tilted = false;
    lis2dh12_acce_value_t acce = {0};

    while (1) {
        if (lis2dh12_get_acce(lis2dh12, &acce) == ESP_OK) {
            float denom = sqrtf(acce.acce_x * acce.acce_x + acce.acce_y * acce.acce_y + acce.acce_z * acce.acce_z);
            if (denom > 0.0001f) {
                float tilt_angle = acosf(acce.acce_z / denom) * 180.0f / (float)M_PI;
                app_state_lock();
                g_state.lis2dh12.tilt_angle = tilt_angle;
                app_state_unlock();

                app_state_t st = {0};
                app_state_snapshot(&st);
                bool new_tilt_state = (tilt_angle < st.lis2dh12.tilt_threshold);
                if (new_tilt_state != is_tilted) {
                    is_tilted = new_tilt_state;
                    if (is_tilted && st.heating.on) {
                        app_event_t ev = {.type = APP_EVENT_FORCE_STOP_HEATING, .reason = APP_STOP_REASON_TILT};
                        (void)app_events_post(&ev, 0);
                    }
                }
            }
        } else {
            ESP_LOGE(TAG, "Failed to get acceleration data");
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void app_service_motion_start(i2c_bus_handle_t i2c_bus) {
    s_i2c_bus = i2c_bus;
    xTaskCreate(lis2dh12_task, "lis2dh12", 4096, NULL, 5, NULL);
}

