#include <math.h>
#include <string.h>

#include "app_fmt.h"
#include "app_state.h"
#include "app_ui.h"
#include "app_usb_pd.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "reflow/reflow_service.h"
#include "ui.h"

#define TAG "ui_render"

static void reflow_chart_draw_tset_to(lv_obj_t *chart, lv_chart_series_t *series, uint8_t profile_id) {
    if (chart == NULL || series == NULL) return;
    reflow_point_t pts[8] = {0};
    if (!reflow_service_get_profile_points(profile_id, pts, 8)) return;

    const uint16_t total_s = pts[7].t_s;
    const int pc = (int)lv_chart_get_point_count(chart);
    if (pc <= 1) return;

    for (int i = 0; i < pc; i++) {
        const float t_s = (total_s == 0) ? 0.0f : ((float)i * (float)total_s / (float)(pc - 1));
        float tset_c = (float)pts[7].temp_c;

        if (t_s <= (float)pts[0].t_s) {
            tset_c = (float)pts[0].temp_c;
        } else if (t_s >= (float)pts[7].t_s) {
            tset_c = (float)pts[7].temp_c;
        } else {
            for (int p = 0; p + 1 < 8; p++) {
                const float t0 = (float)pts[p].t_s;
                const float t1 = (float)pts[p + 1].t_s;
                if (t_s >= t0 && t_s <= t1) {
                    const float y0 = (float)pts[p].temp_c;
                    const float y1 = (float)pts[p + 1].temp_c;
                    const float dt = t1 - t0;
                    const float a = (dt <= 0.0f) ? 1.0f : ((t_s - t0) / dt);
                    tset_c = y0 + a * (y1 - y0);
                    break;
                }
            }
        }

        lv_chart_set_value_by_id(chart, series, i, (int32_t)lroundf(tset_c));
    }
    lv_chart_refresh(chart);
}

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
    char buf_reflow[32];

    uint8_t last_reflow_profile = 0xFF;
    uint32_t last_reflow_revision = 0;
    uint32_t last_reflow_run_id = 0;
    reflow_state_t last_reflow_state = REFLOW_STATE_IDLE;
    int last_reflow_temp_idx = -1;

    while (1) {
        app_state_t st = {0};
        app_state_snapshot(&st);

        reflow_snapshot_t reflow = {0};
        reflow_service_snapshot(&reflow);

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

        // REFLOW UI
        {
            const bool is_running = (reflow.state == REFLOW_STATE_RUNNING);
            const bool is_paused = (reflow.state == REFLOW_STATE_PAUSED);

            // Bottom controls: disable SELECT/EDIT while running, and use START as start/resume/pause.
            if (is_running) {
                lv_obj_add_state(ui_ButtonReflowRunSelect, LV_STATE_DISABLED);
                lv_obj_add_state(ui_ButtonReflowRunEdit, LV_STATE_DISABLED);
                lv_label_set_text(ui_LabelReflowRunStart, "PAUS");
                lv_obj_set_style_bg_color(ui_ButtonReflowRunStart, lv_color_hex(0xAACA4B), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else if (is_paused) {
                lv_obj_clear_state(ui_ButtonReflowRunSelect, LV_STATE_DISABLED);
                lv_obj_clear_state(ui_ButtonReflowRunEdit, LV_STATE_DISABLED);
                lv_label_set_text(ui_LabelReflowRunStart, "RESUME");
                lv_obj_set_style_bg_color(ui_ButtonReflowRunStart, lv_color_hex(0xAACA4B), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else {
                lv_obj_clear_state(ui_ButtonReflowRunSelect, LV_STATE_DISABLED);
                lv_obj_clear_state(ui_ButtonReflowRunEdit, LV_STATE_DISABLED);
                lv_label_set_text(ui_LabelReflowRunStart, "START");
                lv_obj_set_style_bg_color(ui_ButtonReflowRunStart, lv_color_hex(0x1AD1B2), LV_PART_MAIN | LV_STATE_DEFAULT);
            }

            // Redraw Tset when profile points change
            if (reflow.profile_id != last_reflow_profile || reflow.profile_revision != last_reflow_revision) {
                reflow_chart_draw_tset_to(ui_ReflowRunChart, ui_ReflowRunChart_series_tset, reflow.profile_id);
                lv_chart_set_all_value(ui_ReflowRunChart, ui_ReflowRunChart_series_temp, LV_CHART_POINT_NONE);
                last_reflow_temp_idx = -1;
                last_reflow_profile = reflow.profile_id;
                last_reflow_revision = reflow.profile_revision;
            }

            // Clear temp trace on each new run or when returning to IDLE
            if (reflow.run_id != last_reflow_run_id || (last_reflow_state != REFLOW_STATE_IDLE && reflow.state == REFLOW_STATE_IDLE)) {
                lv_chart_set_all_value(ui_ReflowRunChart, ui_ReflowRunChart_series_temp, LV_CHART_POINT_NONE);
                last_reflow_temp_idx = -1;
                last_reflow_run_id = reflow.run_id;
            }
            last_reflow_state = reflow.state;

            // Update temp trace while running/paused
            if ((is_running || is_paused) && isfinite(st.temp.pt1000) && reflow.total_s > 0) {
                const int pc = (int)lv_chart_get_point_count(ui_ReflowRunChart);
                const int idx = (int)((uint32_t)reflow.elapsed_s * (uint32_t)(pc - 1) / (uint32_t)reflow.total_s);
                const int clamped = (idx < 0) ? 0 : ((idx >= pc) ? (pc - 1) : idx);
                if (last_reflow_temp_idx < 0) last_reflow_temp_idx = clamped;

                const int start_i = last_reflow_temp_idx;
                const int end_i = clamped;
                if (end_i >= start_i) {
                    for (int i = start_i; i <= end_i; i++) {
                        lv_chart_set_value_by_id(ui_ReflowRunChart,
                                                 ui_ReflowRunChart_series_temp,
                                                 i,
                                                 (int32_t)lroundf(st.temp.pt1000));
                    }
                    last_reflow_temp_idx = end_i;
                }
            }

            // Labels over the chart
            if (isfinite(st.temp.pt1000)) {
                snprintf(buf_reflow, sizeof(buf_reflow), "%03d/%03d℃",
                         (int)lroundf(st.temp.pt1000),
                         (int)lroundf(reflow.tset_c));
            } else {
                snprintf(buf_reflow, sizeof(buf_reflow), "---/%03d℃", (int)lroundf(reflow.tset_c));
            }
            lv_label_set_text(ui_LabelReflowRunTempTset, buf_reflow);

            snprintf(buf_reflow, sizeof(buf_reflow), "功率:%02dW", (int)lroundf(st.power.power));
            lv_label_set_text(ui_LabelReflowRunPower, buf_reflow);

            snprintf(buf_reflow, sizeof(buf_reflow), "%u/%us", (unsigned)reflow.elapsed_s, (unsigned)reflow.total_s);
            lv_label_set_text(ui_LabelReflowRunTime, buf_reflow);

            const char *st_txt = "空闲";
            if (reflow.state == REFLOW_STATE_RUNNING) st_txt = "加热";
            else if (reflow.state == REFLOW_STATE_PAUSED) st_txt = "暂停";
            lv_label_set_text(ui_LabelReflowRunState, st_txt);
        }

        bsp_display_unlock();

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(100));
    }
}

void app_ui_render_start(void) {
    xTaskCreate(ui_render_task, "ui_render", 4096, NULL, 6, NULL);
}
