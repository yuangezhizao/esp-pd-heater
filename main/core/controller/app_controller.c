#include "app_controller.h"

#include "app_buzzer.h"
#include "app_events.h"
#include "app_lcd_variant.h"
#include "app_state.h"
#include "beep.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "app_controller"

static void controller_handle_event(const app_event_t *event) {
    switch (event->type) {
        case APP_EVENT_SET_HEATING: {
            if (event->value.b) {
                esp_err_t err = app_lcd_variant_lock();
                if (err != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to lock LCD variant: %s", esp_err_to_name(err));
                }
            }
            app_state_set_heating(event->value.b);
            // best-effort: if stopped via UI, callbacks already updated label; if stopped elsewhere, controller handles via FORCE_STOP.
            break;
        }
        case APP_EVENT_FORCE_STOP_HEATING: {
            app_state_set_heating(false);

            app_state_t snapshot;
            app_state_snapshot(&snapshot);

            ESP_LOGW(TAG, "Force stop heating, reason=%d", (int)event->reason);

            // alarm
            if (event->reason == APP_STOP_REASON_TILT) {
                beep(snapshot.config.buzzer_note,
                     calculate_buzzer_volume(snapshot.config.buzzer_volume < 1 ? 1 : snapshot.config.buzzer_volume),
                     0.1,
                     0.02,
                     20);
            } else if (event->reason == APP_STOP_REASON_OVERTEMP) {
                beep(snapshot.config.buzzer_note,
                     calculate_buzzer_volume(snapshot.config.buzzer_volume < 1 ? 1 : snapshot.config.buzzer_volume),
                     0.1,
                     0.05,
                     3);
            } else if (event->reason == APP_STOP_REASON_UNDERVOLTAGE) {
                beep(snapshot.config.buzzer_note,
                     calculate_buzzer_volume(snapshot.config.buzzer_volume < 1 ? 1 : snapshot.config.buzzer_volume),
                     0.1,
                     0.05,
                     5);
            } else {
                beep(snapshot.config.buzzer_note,
                     calculate_buzzer_volume(snapshot.config.buzzer_volume < 1 ? 1 : snapshot.config.buzzer_volume),
                     0.2,
                     0.02,
                     2);
            }
            break;
        }
        case APP_EVENT_SET_PID_TARGET:
            app_state_set_pid_target(event->value.u16);
            break;
        case APP_EVENT_SET_PID_KP:
            app_state_set_pid_kp(event->value.f);
            app_state_update_pid();
            break;
        case APP_EVENT_SET_PID_KI:
            app_state_set_pid_ki(event->value.f);
            app_state_update_pid();
            break;
        case APP_EVENT_SET_PID_KD:
            app_state_set_pid_kd(event->value.f);
            app_state_update_pid();
            break;
        case APP_EVENT_SET_SUPPLY_MAX_POWER:
            app_state_set_supply_max_power(event->value.u8);
            break;
        case APP_EVENT_SET_PCB_R_AT_20C:
            app_state_set_pcb_r_at_20c(event->value.u16);
            break;
        case APP_EVENT_SET_UPPER_DIV_R:
            app_state_set_upper_div_r(event->value.u16);
            break;
        case APP_EVENT_SET_ADC_OFFSET:
            app_state_set_adc_offset(event->value.i16);
            break;
        case APP_EVENT_SET_BRIGHTNESS:
            app_state_set_brightness(event->value.u8);
            bsp_display_brightness_set(event->value.u8, 0);
            break;
        case APP_EVENT_SET_BUZZER_VOLUME:
            app_state_set_buzzer_volume(event->value.u8);
            {
                app_state_t snapshot;
                app_state_snapshot(&snapshot);
                bsp_buzzer_set_default(snapshot.config.buzzer_note, calculate_buzzer_volume(snapshot.config.buzzer_volume));
            }
            break;
        case APP_EVENT_SET_BUZZER_NOTE:
            app_state_set_buzzer_note(event->value.u8);
            {
                app_state_t snapshot;
                app_state_snapshot(&snapshot);
                bsp_buzzer_set_default(snapshot.config.buzzer_note, calculate_buzzer_volume(snapshot.config.buzzer_volume));
            }
            break;
        case APP_EVENT_SET_TILT_THRESHOLD:
            app_state_set_tilt_threshold(event->value.u8);
            break;
        case APP_EVENT_SET_SHUNT_RES:
            app_state_set_shunt_res(event->value.u8);
            break;
        case APP_EVENT_SET_MIN_VOLTAGE:
            app_state_set_min_voltage(event->value.u8);
            break;
        case APP_EVENT_SET_SOFT_START_TIME:
            app_state_set_soft_start_time(event->value.u8);
            break;
        case APP_EVENT_DATA_RECORD_RESTART:
            app_state_set_data_record_restart(true);
            break;
        default:
            ESP_LOGW(TAG, "Unhandled event type: %d", (int)event->type);
            break;
    }
}

static void app_controller_task(void *pvParameter) {
    (void)pvParameter;
    app_event_t event;

    while (1) {
        if (app_events_receive(&event, portMAX_DELAY)) {
            controller_handle_event(&event);
        }
    }
}

void app_controller_start(void) {
    app_events_init();
    xTaskCreate(app_controller_task, "app_controller", 4096, NULL, 9, NULL);
}
