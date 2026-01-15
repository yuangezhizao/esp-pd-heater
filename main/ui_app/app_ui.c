#include "app_ui.h"

#include <math.h>

#include "app_buzzer.h"
#include "app_events.h"
#include "app_state.h"
#include "app_usb_pd.h"
#include "bsp/esp-bsp.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "ui.h"

#define TAG "app_ui"

// chart
lv_chart_series_t *ui_Chart1_series_1 = NULL;
lv_chart_series_t *ui_Chart1_series_2 = NULL;

// 在文件开头添加宏定义
#define SLIDER_SCROLL_MIN_INTERVAL 10   // min time interval between scrolls (ms)
#define SLIDER_SCROLL_MAX_INTERVAL 200  // max time interval between scrolls (ms)
#define SLIDER_SCROLL_MIN_STEP_SIZE 1
#define SLIDER_SCROLL_MAX_STEP_SIZE 10

// 定义一个结构体来管理 slider 的快速滚动状态
typedef struct {
    uint32_t last_tick;
    int16_t last_value;
    bool is_init;  // 添加初始化标志
} slider_scroll_state_t;

// 为每个需要快速滚动的 slider 创建状态变量
static slider_scroll_state_t temp_slider_state;
static slider_scroll_state_t power_slider_state;
static slider_scroll_state_t bl_slider_state;
static slider_scroll_state_t vol_slider_state;

// 获取滚动步长的函数
static int slider_scroll_speed_get_step(slider_scroll_state_t *state) {
    uint32_t interval = lv_tick_elaps(state->last_tick);
    state->last_tick = lv_tick_get();

    if (interval <= SLIDER_SCROLL_MIN_INTERVAL) {
        return SLIDER_SCROLL_MAX_STEP_SIZE;
    } else if (interval >= SLIDER_SCROLL_MAX_INTERVAL) {
        return SLIDER_SCROLL_MIN_STEP_SIZE;
    }

    float factor = (float)(interval - SLIDER_SCROLL_MIN_INTERVAL) / (SLIDER_SCROLL_MAX_INTERVAL - SLIDER_SCROLL_MIN_INTERVAL);
    int step = (int)((1.0 - factor) * (SLIDER_SCROLL_MAX_STEP_SIZE - SLIDER_SCROLL_MIN_STEP_SIZE));

    return step > 0 ? step : 1;
}

// 通用的slider快速滚动处理函数
static int handle_slider_fast_scroll(slider_scroll_state_t *state, lv_obj_t *slider, int current_value) {
    // 第一次使用时初始化last_value
    if (!state->is_init) {
        state->last_value = current_value;
        state->is_init = true;
        return current_value;
    }

    int step = slider_scroll_speed_get_step(state);
    int diff = current_value - state->last_value;

    if (diff != 0) {
        if (diff > 0) {
            current_value = state->last_value + step;
        } else {
            current_value = state->last_value - step;
        }

        // 确保值在有效范围内
        if (current_value < lv_slider_get_min_value(slider)) {
            current_value = lv_slider_get_min_value(slider);
        }
        if (current_value > lv_slider_get_max_value(slider)) {
            current_value = lv_slider_get_max_value(slider);
        }
        lv_obj_set_style_anim_time(slider, 50, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_slider_set_value(slider, current_value, LV_ANIM_ON);
        state->last_value = current_value;
    }

    return current_value;
}

static void button_heating_toggle_event_cb(lv_event_t *e) {
    lv_obj_t *btn = lv_event_get_target(e);

    // 电压小于 min_voltage 禁止开启加热
    app_state_t st = {0};
    app_state_snapshot(&st);
    if (st.power.voltage < st.power.min_voltage) {
        app_event_t ev = {.type = APP_EVENT_SET_HEATING};
        ev.value.b = false;
        (void)app_events_post(&ev, 0);

        bsp_display_lock(0);
        lv_obj_clear_state(btn, LV_STATE_CHECKED);
        lv_obj_set_style_text_decor(ui_LabelHeatingToggle, LV_TEXT_DECOR_STRIKETHROUGH, LV_PART_MAIN | LV_STATE_DEFAULT);
        bsp_display_unlock();

        // 发出警报
        beep(st.config.buzzer_note, calculate_buzzer_volume(st.config.buzzer_volume), 0.2, 0.01, 2);
        return;
    }

    bool heating_on = lv_obj_has_state(btn, LV_STATE_CHECKED);
    app_event_t ev = {.type = APP_EVENT_SET_HEATING};
    ev.value.b = heating_on;
    (void)app_events_post(&ev, 0);

    bsp_display_lock(0);
    lv_label_set_text(ui_LabelHeatingToggle, heating_on ? "STOP" : "START");
    lv_obj_set_style_text_decor(ui_LabelHeatingToggle, LV_TEXT_DECOR_NONE, LV_PART_MAIN | LV_STATE_DEFAULT);
    bsp_display_unlock();
}

static void slider_set_temp_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        int current_value = lv_slider_get_value(slider);
        current_value = handle_slider_fast_scroll(&temp_slider_state, slider, current_value);

        // 更新状态和显示
        app_event_t ev = {.type = APP_EVENT_SET_PID_TARGET};
        ev.value.u16 = (uint16_t)current_value;
        (void)app_events_post(&ev, 0);

        char buf_set_temp[16];
        snprintf(buf_set_temp, sizeof(buf_set_temp), "%d℃", current_value);
        bsp_display_lock(0);
        lv_label_set_text(ui_LabelSetTemp, buf_set_temp);

        // test ui
        char buf0[16];
        snprintf(buf0, sizeof(buf0), "%d℃", current_value);
        lv_label_set_text(ui_LabelRealTemp3, buf0);

        bsp_display_unlock();
    }
}

static void slider_set_max_power_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        int current_value = lv_slider_get_value(slider);
        current_value = handle_slider_fast_scroll(&power_slider_state, slider, current_value);

        // 更新状态和显示
        app_event_t ev = {.type = APP_EVENT_SET_SUPPLY_MAX_POWER};
        ev.value.u8 = (uint8_t)current_value;
        (void)app_events_post(&ev, 0);

        char buf_set_max_power[16];
        snprintf(buf_set_max_power, sizeof(buf_set_max_power), "%dW", current_value);
        bsp_display_lock(0);
        lv_label_set_text(ui_LabelSetMaxPower, buf_set_max_power);
        bsp_display_unlock();
    }
}

static void slider_set_rpcb_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int current_value = lv_slider_get_value(slider);
    app_event_t ev = {.type = APP_EVENT_SET_PCB_R_AT_20C};
    ev.value.u16 = (uint16_t)current_value;
    (void)app_events_post(&ev, 0);

    char buf_set_pcb_r[16];
    snprintf(buf_set_pcb_r, sizeof(buf_set_pcb_r), "%.2fR", (float)current_value / 100.0f);
    bsp_display_lock(0);
    lv_label_set_text(ui_LabelSetRPcb, buf_set_pcb_r);
    bsp_display_unlock();
}

static void slider_set_rdiv_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int current_value = lv_slider_get_value(slider);
    app_event_t ev = {.type = APP_EVENT_SET_UPPER_DIV_R};
    ev.value.u16 = (uint16_t)current_value;
    (void)app_events_post(&ev, 0);

    char buf_set_rdiv[16];
    snprintf(buf_set_rdiv, sizeof(buf_set_rdiv), "%dR", current_value);
    bsp_display_lock(0);
    lv_label_set_text(ui_LabelSetRDiv, buf_set_rdiv);
    bsp_display_unlock();
}

static void slider_set_adc_offset_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int current_value = lv_slider_get_value(slider);
    app_event_t ev = {.type = APP_EVENT_SET_ADC_OFFSET};
    ev.value.i16 = (int16_t)current_value;
    (void)app_events_post(&ev, 0);

    char buf_set_adcoffset[16];
    snprintf(buf_set_adcoffset, sizeof(buf_set_adcoffset), "%+dmV", current_value);
    bsp_display_lock(0);
    lv_label_set_text(ui_LabelSetADCOffset, buf_set_adcoffset);
    bsp_display_unlock();
}

static void slider_set_bl_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        int current_value = lv_slider_get_value(slider);
        current_value = handle_slider_fast_scroll(&bl_slider_state, slider, current_value);

        // 更新状态和显示
        app_event_t ev = {.type = APP_EVENT_SET_BRIGHTNESS};
        ev.value.u8 = (uint8_t)current_value;
        (void)app_events_post(&ev, 0);

        char buf_set_bl[16];
        snprintf(buf_set_bl, sizeof(buf_set_bl), "%d%%", current_value);
        bsp_display_lock(0);
        lv_label_set_text(ui_LabelSetBL, buf_set_bl);
        bsp_display_unlock();
    }
}

static void slider_set_vol_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED) {
        int current_value = lv_slider_get_value(slider);
        current_value = handle_slider_fast_scroll(&vol_slider_state, slider, current_value);

        // 更新状态和显示
        app_event_t ev = {.type = APP_EVENT_SET_BUZZER_VOLUME};
        ev.value.u8 = (uint8_t)current_value;
        (void)app_events_post(&ev, 0);

        char buf_set_vol[16];
        snprintf(buf_set_vol, sizeof(buf_set_vol), "%d%%", current_value);
        bsp_display_lock(0);
        lv_label_set_text(ui_LabelSetVol, buf_set_vol);
        bsp_display_unlock();
    }
}

static void slider_set_note_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int current_value = lv_slider_get_value(slider);
    app_event_t ev = {.type = APP_EVENT_SET_BUZZER_NOTE};
    ev.value.u8 = (uint8_t)current_value;
    (void)app_events_post(&ev, 0);

    char buf_set_note[16];
    snprintf(buf_set_note, sizeof(buf_set_note), "%d", current_value);
    bsp_display_lock(0);
    lv_label_set_text(ui_LabelSetNote, buf_set_note);
    bsp_display_unlock();
}

static void slider_set_kp_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    uint16_t value = lv_slider_get_value(slider);
    app_event_t ev = {.type = APP_EVENT_SET_PID_KP};
    ev.value.f = (float)value;
    (void)app_events_post(&ev, 0);

    char buf_set_kp[16];
    snprintf(buf_set_kp, sizeof(buf_set_kp), "%d", value);
    bsp_display_lock(0);
    lv_label_set_text(ui_LabelSetKp, buf_set_kp);
    bsp_display_unlock();
}

static void slider_set_ki_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    uint16_t value = lv_slider_get_value(slider);
    app_event_t ev = {.type = APP_EVENT_SET_PID_KI};
    // UI slider is integer-only. Map to a small Ki range suitable for positional PID.
    // With PID_LOOP_PERIOD_MS=100ms and pid_ctrl's integral not scaled by dt, Ki must be small.
    ev.value.f = (float)value / 1000.0f; // 1 slider step = 0.001
    (void)app_events_post(&ev, 0);

    char buf_set_ki[16];
    snprintf(buf_set_ki, sizeof(buf_set_ki), "%.3f", (double)ev.value.f);
    bsp_display_lock(0);
    lv_label_set_text(ui_LabelSetKi, buf_set_ki);
    bsp_display_unlock();
}

static void slider_set_kd_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    uint16_t value = lv_slider_get_value(slider);
    app_event_t ev = {.type = APP_EVENT_SET_PID_KD};
    ev.value.f = (float)value;
    (void)app_events_post(&ev, 0);

    char buf_set_kd[16];
    snprintf(buf_set_kd, sizeof(buf_set_kd), "%d", value);
    bsp_display_lock(0);
    lv_label_set_text(ui_LabelSetKd, buf_set_kd);
    bsp_display_unlock();
}

static void chart_draw_event_cb(lv_event_t *e) {
    lv_obj_draw_part_dsc_t *dsc = lv_event_get_draw_part_dsc(e);
    if (!lv_obj_draw_part_check_type(dsc, &lv_chart_class, LV_CHART_DRAW_PART_TICK_LABEL))
        return;
    if (dsc->text == NULL)
        return;

    if (dsc->id == LV_CHART_AXIS_PRIMARY_X) {
        const char *px[] = {"0", "", "20", "", "40", "", "60", "", "80", "", "100", "", "120"};
        lv_snprintf(dsc->text, dsc->text_length, "%s", px[dsc->value]);
    }
    if (dsc->id == LV_CHART_AXIS_SECONDARY_Y) {
        lv_snprintf(dsc->text, dsc->text_length, "%d", dsc->value / 100);
    }
}

static void chart_clicked_event_cb(lv_event_t *e) {
    app_event_t ev = {.type = APP_EVENT_DATA_RECORD_RESTART};
    (void)app_events_post(&ev, 0);
}

static void slider_set_tilt_threshold_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int current_value = lv_slider_get_value(slider);
    app_event_t ev = {.type = APP_EVENT_SET_TILT_THRESHOLD};
    ev.value.u8 = (uint8_t)current_value;
    (void)app_events_post(&ev, 0);

    char buf_set_tilt[16];
    snprintf(buf_set_tilt, sizeof(buf_set_tilt), "%d°", current_value);
    bsp_display_lock(0);
    lv_label_set_text(ui_LabelSetTiltThreshold, buf_set_tilt);
    bsp_display_unlock();
}

static void slider_set_shunt_r_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int current_value = lv_slider_get_value(slider);
    app_event_t ev = {.type = APP_EVENT_SET_SHUNT_RES};
    ev.value.u8 = (uint8_t)current_value;
    (void)app_events_post(&ev, 0);

    char buf_set_shunt_r[16];
    snprintf(buf_set_shunt_r, sizeof(buf_set_shunt_r), "%dmR", current_value);
    bsp_display_lock(0);
    lv_label_set_text(ui_LabelSetShuntR, buf_set_shunt_r);
    bsp_display_unlock();
}

static void slider_set_min_volt_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int current_value = lv_slider_get_value(slider);
    app_event_t ev = {.type = APP_EVENT_SET_MIN_VOLTAGE};
    ev.value.u8 = (uint8_t)current_value;
    (void)app_events_post(&ev, 0);

    char buf_set_min_volt[16];
    snprintf(buf_set_min_volt, sizeof(buf_set_min_volt), "%dV", current_value);
    bsp_display_lock(0);
    lv_label_set_text(ui_LabelSetMinVolt, buf_set_min_volt);
    bsp_display_unlock();
}

static void slider_set_soft_start_time_event_cb(lv_event_t *e) {
    lv_obj_t *slider = lv_event_get_target(e);
    int current_value = lv_slider_get_value(slider);
    if (current_value < 0) current_value = 0;
    if (current_value > 100) current_value = 100;

    app_event_t ev = {.type = APP_EVENT_SET_SOFT_START_TIME};
    ev.value.u8 = (uint8_t)current_value;
    (void)app_events_post(&ev, 0);

    char buf[16];
    snprintf(buf, sizeof(buf), "%ds", current_value);
    bsp_display_lock(0);
    lv_label_set_text(ui_LabelSetSoftStartTime, buf);
    bsp_display_unlock();
}

void app_lvgl_display(void) {
    bsp_display_lock(0);

    lv_group_t *g = lv_group_create();
    lv_indev_set_group(bsp_display_get_input_dev(), g);
    lv_group_set_wrap(g, false);  // 禁用循环滚动

    ui_init();

    lv_disp_t *dispp = lv_disp_get_default();
    // lv_theme_t *theme = lv_theme_default_init(dispp, lv_color_hex(0x76DAFF), lv_color_hex(0xF784B6), true, LV_FONT_DEFAULT);
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_color_hex(0x5299B3), lv_color_hex(0xF784B6), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);

    lv_obj_set_scroll_snap_x(ui_Screen1, LV_SCROLL_SNAP_CENTER);
    // lv_obj_set_scroll_snap_y(ui_Screen1, LV_SCROLL_SNAP_CENTER);

    // Page -1
    lv_chart_set_update_mode(ui_Chart1, LV_CHART_UPDATE_MODE_SHIFT);
    ui_Chart1_series_1 = lv_chart_add_series(ui_Chart1, lv_color_hex(0x85BFD5), LV_CHART_AXIS_PRIMARY_Y);
    // ui_Chart1_series_2 = lv_chart_add_series(ui_Chart1, lv_color_hex(0xF784B6), LV_CHART_AXIS_SECONDARY_Y);
    ui_Chart1_series_2 = lv_chart_add_series(ui_Chart1, lv_color_hex(0xCD6D96), LV_CHART_AXIS_SECONDARY_Y);
    lv_obj_add_event_cb(ui_Chart1, chart_draw_event_cb, LV_EVENT_DRAW_PART_BEGIN, NULL);
    lv_group_add_obj(g, ui_ButtonChartRestart);
    lv_obj_add_event_cb(ui_ButtonChartRestart, chart_clicked_event_cb, LV_EVENT_CLICKED, NULL);
    // Page 0
    lv_group_add_obj(g, ui_ButtonHeatingToggle);
    // lv_obj_add_event_cb(ui_ButtonHeatingToggle, button_heating_toggle_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    // lv_obj_scroll_to_view_recursive(ui_ButtonHeatingToggle, LV_ANIM_OFF);
    // lv_group_focus_obj(ui_ButtonHeatingToggle);
    // Page -2
    lv_group_add_obj(g, ui_ContainerPageM2);
    lv_obj_add_event_cb(ui_ContainerPageM2, button_heating_toggle_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_scroll_to_view_recursive(ui_ContainerPageM2, LV_ANIM_OFF);
    lv_group_focus_obj(ui_ContainerPageM2);
    // Page 1
    lv_group_add_obj(g, ui_SliderSetTemp);
    lv_obj_add_event_cb(ui_SliderSetTemp, slider_set_temp_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetTemp, NULL, LV_STATE_EDITED);
    lv_group_add_obj(g, ui_SliderSetKp);
    lv_obj_add_event_cb(ui_SliderSetKp, slider_set_kp_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetKp, NULL, LV_STATE_EDITED);
    lv_group_add_obj(g, ui_SliderSetKi);
    lv_obj_add_event_cb(ui_SliderSetKi, slider_set_ki_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetKi, NULL, LV_STATE_EDITED);
    lv_group_add_obj(g, ui_SliderSetKd);
    lv_obj_add_event_cb(ui_SliderSetKd, slider_set_kd_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetKd, NULL, LV_STATE_EDITED);
    // Page 2
    lv_group_add_obj(g, ui_SliderSetMaxPower);
    lv_obj_add_event_cb(ui_SliderSetMaxPower, slider_set_max_power_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetMaxPower, NULL, LV_STATE_EDITED);
    lv_group_add_obj(g, ui_SliderSetRPcb);
    lv_obj_add_event_cb(ui_SliderSetRPcb, slider_set_rpcb_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetRPcb, NULL, LV_STATE_EDITED);
    lv_group_add_obj(g, ui_SliderSetRDiv);
    lv_obj_add_event_cb(ui_SliderSetRDiv, slider_set_rdiv_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetRDiv, NULL, LV_STATE_EDITED);
    lv_group_add_obj(g, ui_SliderSetADCOffset);
    lv_obj_add_event_cb(ui_SliderSetADCOffset, slider_set_adc_offset_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetADCOffset, NULL, LV_STATE_EDITED);
    // Page 4
    lv_group_add_obj(g, ui_SliderSetMinVolt);
    lv_obj_add_event_cb(ui_SliderSetMinVolt, slider_set_min_volt_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetMinVolt, NULL, LV_STATE_EDITED);
    lv_group_add_obj(g, ui_SliderSetShuntR);
    lv_obj_add_event_cb(ui_SliderSetShuntR, slider_set_shunt_r_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetShuntR, NULL, LV_STATE_EDITED);
    lv_group_add_obj(g, ui_SliderSetSoftStartTime);
    lv_obj_add_event_cb(ui_SliderSetSoftStartTime, slider_set_soft_start_time_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetSoftStartTime, NULL, LV_STATE_EDITED);
    // Page 3
    lv_group_add_obj(g, ui_SliderSetBL);
    lv_obj_add_event_cb(ui_SliderSetBL, slider_set_bl_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetBL, NULL, LV_STATE_EDITED);
    lv_group_add_obj(g, ui_SliderSetVol);
    lv_obj_add_event_cb(ui_SliderSetVol, slider_set_vol_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetVol, NULL, LV_STATE_EDITED);
    lv_group_add_obj(g, ui_SliderSetNote);
    lv_obj_add_event_cb(ui_SliderSetNote, slider_set_note_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetNote, NULL, LV_STATE_EDITED);
    lv_group_add_obj(g, ui_SliderSetTiltThreshold);
    lv_obj_add_event_cb(ui_SliderSetTiltThreshold, slider_set_tilt_threshold_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_remove_style(ui_SliderSetTiltThreshold, NULL, LV_STATE_EDITED);
    // Page 5
    // lv_group_add_obj(g, ui_Panel1);
    lv_group_add_obj(g, ui_ContainerPage5);

    bsp_display_unlock();

    // UI background tasks (single writer for LVGL updates)
    app_ui_render_start();
    app_ui_data_record_start();
}

void update_slider_and_label(lv_obj_t *label, lv_obj_t *slider, const char *format, int value) {
    char buf[32];
    snprintf(buf, sizeof(buf), format, value);
    lv_label_set_text(label, buf);
    lv_slider_set_value(slider, value, LV_ANIM_OFF);
}

void lvgl_widgets_values_init(void) {
    app_state_t st = {0};
    app_state_snapshot(&st);

    bsp_display_lock(0);

    // test ui
    char buf0[16];
    snprintf(buf0, sizeof(buf0), "%d℃", st.temp.pid_target);
    lv_label_set_text(ui_LabelRealTemp3, buf0);

    update_slider_and_label(ui_LabelSetKp, ui_SliderSetKp, "%d", (uint16_t)st.temp.pid.kp);
    update_slider_and_label(ui_LabelSetKd, ui_SliderSetKd, "%d", (uint16_t)st.temp.pid.kd);
    update_slider_and_label(ui_LabelSetTemp, ui_SliderSetTemp, "%d℃", st.temp.pid_target);
    update_slider_and_label(ui_LabelSetMaxPower, ui_SliderSetMaxPower, "%dW", st.heating.supply_max_power);
    update_slider_and_label(ui_LabelSetRDiv, ui_SliderSetRDiv, "%dR", st.adc.upper_div_r);
    update_slider_and_label(ui_LabelSetADCOffset, ui_SliderSetADCOffset, "%+dmV", st.adc.offset);
    update_slider_and_label(ui_LabelSetBL, ui_SliderSetBL, "%d%%", st.config.brightness);
    update_slider_and_label(ui_LabelSetVol, ui_SliderSetVol, "%d%%", st.config.buzzer_volume);
    update_slider_and_label(ui_LabelSetNote, ui_SliderSetNote, "%d", st.config.buzzer_note);
    update_slider_and_label(ui_LabelSetTiltThreshold, ui_SliderSetTiltThreshold, "%d°", st.lis2dh12.tilt_threshold);
    update_slider_and_label(ui_LabelSetShuntR, ui_SliderSetShuntR, "%dmR", st.power.shunt_res);
    update_slider_and_label(ui_LabelSetMinVolt, ui_SliderSetMinVolt, "%dV", st.power.min_voltage);
    update_slider_and_label(ui_LabelSetSoftStartTime, ui_SliderSetSoftStartTime, "%ds", (int)st.heating.soft_start_time_s);

    // PID Ki uses fractional value; show 3 decimals and map to the integer slider (x1000).
    char buf_ki[16];
    snprintf(buf_ki, sizeof(buf_ki), "%.3f", (double)st.temp.pid.ki);
    lv_label_set_text(ui_LabelSetKi, buf_ki);
    int ki_slider = (int)lroundf(st.temp.pid.ki * 1000.0f);
    if (ki_slider < 0) ki_slider = 0;
    if (ki_slider > 300) ki_slider = 300; // matches ui_SliderSetKi range in generated UI
    lv_slider_set_value(ui_SliderSetKi, ki_slider, LV_ANIM_OFF);

    // pcb_r_at_20c float 特殊处理一下
    char buf[16];
    snprintf(buf, sizeof(buf), "%.2fR", (float)st.heating.pcb_r_at_20c / 100.0f);
    lv_label_set_text(ui_LabelSetRPcb, buf);
    lv_slider_set_value(ui_SliderSetRPcb, st.heating.pcb_r_at_20c, LV_ANIM_OFF);

    bsp_display_unlock();
}
