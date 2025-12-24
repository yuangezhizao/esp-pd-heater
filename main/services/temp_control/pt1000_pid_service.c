#include "app_services.h"

#include <math.h>

#include "app_buzzer.h"
#include "app_power_limit.h"
#include "app_pt1000.h"
#include "app_state.h"
#include "beep.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "pid_ctrl.h"

#define TAG "pt1000_pid"

#define ADC_CHAN ADC_CHANNEL_4
#define ADC_ATTEN ADC_ATTEN_DB_6

// Control-loop period should remain stable for consistent tuning.
#define PID_LOOP_PERIOD_MS 100

static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_cali_handle_t s_adc_cali_handle = NULL;

static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    if (!calibrated) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "ADC calibration success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "ADC calibration init failed: %s", esp_err_to_name(ret));
    }

    return calibrated;
}

static void adc_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &s_adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, ADC_CHAN, &config));

    (void)adc_calibration_init(ADC_UNIT_1, ADC_CHAN, ADC_ATTEN, &s_adc_cali_handle);
}

static void pid_init(void) {
    app_state_t st = {0};
    app_state_snapshot(&st);
    pid_ctrl_config_t pid_config = {
        .init_param = st.temp.pid,
    };
    pid_ctrl_block_handle_t pid_ctrl = NULL;
    ESP_ERROR_CHECK(pid_new_control_block(&pid_config, &pid_ctrl));
    app_state_lock();
    g_state.temp.pid_ctrl = pid_ctrl;
    app_state_unlock();
}

static void pt1000_pid_task(void *pvParameter) {
    (void)pvParameter;

    adc_init();

    int adc_raw_value;
    int voltage;

    const size_t sample_count = 64;
    double voltage_average = 0.0f;

    double pt1000_resistance;
    float pid_temperature_error;
    float pid_output_value;

    app_state_t st = {0};
    bool last_heating_on = false;
    float last_max_output = -1.0f;

    bool temp_filter_inited = false;
    float temp_filtered = 0.0f;
    // Small low-pass on temperature to reduce ADC noise without adding too much lag.
    const float temp_filter_alpha = 0.2f;

    TickType_t last_wake_tick = xTaskGetTickCount();
    uint32_t loop_count = 0;

    while (1) {
        voltage_average = 0;
        for (size_t i = 0; i < sample_count; i++) {
            ESP_ERROR_CHECK(adc_oneshot_read(s_adc_handle, ADC_CHAN, &adc_raw_value));
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(s_adc_cali_handle, adc_raw_value, &voltage));
            voltage_average += voltage;
        }
        voltage_average /= sample_count;

        app_state_snapshot(&st);
        voltage_average += st.adc.offset;

        // TODO: make divider VIN configurable if needed; current hardware assumes ~3.0V.
        const double divider_vin_mv = 3000.0;
        const double denom = divider_vin_mv - voltage_average;
        if (denom <= 1.0) {
            // Avoid divide-by-zero; report an obviously invalid temperature.
            pt1000_resistance = NAN;
        } else {
            pt1000_resistance = (voltage_average * st.adc.upper_div_r) / denom;
        }
        const float pt1000_temp_raw = isnan(pt1000_resistance) ? NAN : (float)calculate_temperature_cvd(pt1000_resistance);
        const float pt1000_temp = (!temp_filter_inited || isnan(pt1000_temp_raw))
                                      ? pt1000_temp_raw
                                      : (temp_filter_alpha * pt1000_temp_raw + (1.0f - temp_filter_alpha) * temp_filtered);
        if (!temp_filter_inited && !isnan(pt1000_temp_raw)) {
            temp_filtered = pt1000_temp_raw;
            temp_filter_inited = true;
        } else if (!isnan(pt1000_temp)) {
            temp_filtered = pt1000_temp;
        }

        pid_temperature_error = (float)st.temp.pid_target - pt1000_temp;
        const bool temp_ok = isfinite(pt1000_temp) && isfinite(pid_temperature_error);

        if (st.heating.on && temp_ok) {
            if (!last_heating_on) {
                // Reset PID accumulation when heating starts to avoid using stale integral/derivative history.
                app_state_lock();
                pid_reset_ctrl_block(g_state.temp.pid_ctrl);
                app_state_unlock();
                last_max_output = -1.0f;
            }

            float power_limit_value = power_limit(st.power.voltage,
                                                st.heating.supply_max_power,
                                                st.heating.pcb_r_at_20c / 100.0f,
                                                pt1000_temp,
                                                st.heating.start_time,
                                                st.power.power,
                                                st.heating.pwm_current_duty,
                                                st.heating.pwm_max_duty);
            float max_output = (float)st.heating.pwm_max_duty * power_limit_value;
            if (fabsf(max_output - last_max_output) >= 1.0f) {
                app_state_lock();
                g_state.temp.pid.max_output = max_output;
                app_state_unlock();
                app_state_update_pid();
                last_max_output = max_output;
            }

            app_state_lock();
            pid_compute(g_state.temp.pid_ctrl, pid_temperature_error, &pid_output_value);
            app_state_unlock();
        } else {
            pid_output_value = 0;
            if (last_heating_on) {
                app_state_lock();
                pid_reset_ctrl_block(g_state.temp.pid_ctrl);
                app_state_unlock();
            }
        }
        last_heating_on = st.heating.on;

        uint32_t pwm_current_duty = (uint32_t)roundf(pid_output_value);
        app_state_lock();
        g_state.temp.pt1000 = pt1000_temp;
        g_state.heating.pwm_current_duty = pwm_current_duty;
        g_state.adc.voltage = (float)voltage_average;
        app_state_unlock();

        loop_count++;
        if (st.heating.on && (loop_count % (1000 / PID_LOOP_PERIOD_MS) == 0)) {
            ESP_LOGD(TAG,
                     "Traw=%.2fC T=%.2fC target=%u err=%.2f max=%.1f duty=%u",
                     pt1000_temp_raw,
                     pt1000_temp,
                     (unsigned)st.temp.pid_target,
                     pid_temperature_error,
                     (double)last_max_output,
                     (unsigned)pwm_current_duty);
        }

        // First time temperature reaches target after heating started: beep once.
        bool is_temp_reached = st.heating.is_temp_reached;
        if (st.heating.on && !is_temp_reached && (pid_temperature_error >= -1 && pid_temperature_error <= 0)) {
            is_temp_reached = true;
            uint8_t vol = st.config.buzzer_volume < 1 ? 1 : st.config.buzzer_volume;
            beep(st.config.buzzer_note, calculate_buzzer_volume(vol), 1, 0, 1);
        }
        if (!st.heating.on) is_temp_reached = false;
        app_state_lock();
        g_state.heating.is_temp_reached = is_temp_reached;
        app_state_unlock();

        vTaskDelayUntil(&last_wake_tick, pdMS_TO_TICKS(PID_LOOP_PERIOD_MS));
    }
}

void app_service_temp_control_start(void) {
    pid_init();
    xTaskCreate(pt1000_pid_task, "pt1000_pid", 4096, NULL, 1, NULL);
}
