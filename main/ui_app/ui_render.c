#include <math.h>
#include <string.h>

#include "app_fmt.h"
#include "app_state.h"
#include "app_usb_pd.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "ui.h"

#define TAG "ui_render"

static void apply_heating_ui_off(bool undervoltage) {
    lv_obj_clear_state(ui_ButtonHeatingToggle, LV_STATE_CHECKED);
    lv_obj_clear_state(ui_ContainerPageM2, LV_STATE_CHECKED);
    lv_label_set_text(ui_LabelHeatingToggle, "START");
    lv_obj_set_style_text_decor(ui_LabelHeatingToggle,
                                undervoltage ? LV_TEXT_DECOR_STRIKETHROUGH : LV_TEXT_DECOR_NONE,
                                LV_PART_MAIN | LV_STATE_DEFAULT);
}

static void ui_render_task(void *pvParameter) {
    (void)pvParameter;

    TickType_t last_wake = xTaskGetTickCount();
    char pd_text[512] = {0};
    char pd_text_last[512] = {0};

    char buf_voltage[16];
    char buf_current[16];
    char buf_power[16];
    char buf_temp[16];

    while (1) {
        app_state_t st = {0};
        app_state_snapshot(&st);

        bool undervoltage = (st.power.voltage < (float)st.power.min_voltage);

        (void)app_pd_get_info_text_copy(pd_text, sizeof(pd_text));

        bsp_display_lock(0);

        // Heating button: force OFF visuals when heating is off (especially for force-stop paths).
        if (!st.heating.on) {
            apply_heating_ui_off(undervoltage);
        } else {
            // Clear strikethrough while heating is on.
            lv_obj_set_style_text_decor(ui_LabelHeatingToggle, LV_TEXT_DECOR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
        }

        // Real voltage/current/power (test UI widgets)
        format_float_to_width(buf_voltage, sizeof(buf_voltage), 4, st.power.voltage, "V");
        lv_label_set_text(ui_LabelRealVoltage1, buf_voltage);
        format_float_to_width(buf_current, sizeof(buf_current), 4, st.power.current, "A");
        lv_label_set_text(ui_LabelRealCurrent1, buf_current);
        format_float_to_width(buf_power, sizeof(buf_power), 4, st.power.power, "W");
        lv_label_set_text(ui_LabelRealPower1, buf_power);

        // Duty bar + temperature split labels
        float duty_percent = 0.0f;
        if (st.heating.pwm_max_duty > 0) {
            duty_percent = (float)st.heating.pwm_current_duty * 100.0f / (float)st.heating.pwm_max_duty;
        }
        lv_bar_set_value(ui_BarRealDuty, (int32_t)lroundf(duty_percent), LV_ANIM_ON);

        float temp = st.temp.pt1000;
        if (temp >= 0) {
            snprintf(buf_temp, sizeof(buf_temp), "%03d", (int)temp);
        } else {
            snprintf(buf_temp, sizeof(buf_temp), "-%02d", abs((int)temp));
        }
        lv_label_set_text(ui_LabelRealTemp1, buf_temp);
        snprintf(buf_temp, sizeof(buf_temp), "%d", abs((int)(fmodf(temp * 10.0f, 10.0f))));
        lv_label_set_text(ui_LabelRealTemp2, buf_temp);

        // PD info: update only when content changes (reduce LVGL heap churn)
        if (strncmp(pd_text, pd_text_last, sizeof(pd_text_last)) != 0) {
            strncpy(pd_text_last, pd_text, sizeof(pd_text_last) - 1);
            pd_text_last[sizeof(pd_text_last) - 1] = '\0';
            lv_label_set_text(ui_LabelPDInfo, pd_text_last);
        }

        bsp_display_unlock();

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(100));
    }
}

void app_ui_render_start(void) {
    xTaskCreate(ui_render_task, "ui_render", 4096, NULL, 6, NULL);
}
