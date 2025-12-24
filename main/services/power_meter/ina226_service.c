#include "app_services.h"

#include <math.h>

#include "app_events.h"
#include "app_state.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ina226.h"

#define TAG "ina226_service"

static i2c_bus_handle_t s_i2c_bus = NULL;

static void ina226_task(void *pvParameter) {
    (void)pvParameter;

    ina226_init(s_i2c_bus, 0x40);

    app_state_t st = {0};
    app_state_snapshot(&st);
    float shunt_res_ohm = (float)st.power.shunt_res / 1000.0f;
    ina226_calibrate(shunt_res_ohm, 15);
    // PWM heater: use faster conversions + moderate averaging so power limiting can react quickly.
    // Internal update time ≈ (Tshunt + Tbus) * AVG = (332us + 332us) * 16 ≈ 10.6ms.
    ina226_configure(INA226_PERIOD_332us, INA226_AVERAGE_16);

    bool uv_stop_posted = false;
    float r20_i_ewma_ohm = NAN;
    float r20_p_ewma_ohm = NAN;
    uint32_t r20_samples = 0;
    int64_t pcb_r20_last_log_us = 0;

    while (1) {
        // Wait for a fresh conversion (clears CVRF via register read).
        // This ensures we don't repeatedly consume the same averaged sample.
        if (st.heating.on) {
            while (!ina226_conversion_ready()) {
                vTaskDelay(pdMS_TO_TICKS(1));
            }
        }

        float bus_voltage = 0;
        float current = 0;
        float power = 0;
        float shunt_voltage = 0;
        ina226_read(&bus_voltage, &current, &power, &shunt_voltage);
        current = fabsf(current);

        app_state_snapshot(&st);
        if (!st.heating.on || st.heating.pwm_current_duty == 0) {
            if (current < 0.02f) {
                current = 0;
                power = 0;
            }
        }

        app_state_lock();
        g_state.power.voltage = bus_voltage;
        g_state.power.current = current;
        g_state.power.power = power;
        g_state.power.shunt_voltage = shunt_voltage;
        app_state_unlock();

        // Runtime undervoltage protection: stop heating if supply drops below configured minimum.
        if (st.heating.on && bus_voltage < (float)st.power.min_voltage) {
            if (!uv_stop_posted) {
                uv_stop_posted = true;
                ESP_LOGW(TAG, "Undervoltage: %.2fV < %uV, force stopping heating", bus_voltage, (unsigned)st.power.min_voltage);
                app_event_t ev = {.type = APP_EVENT_FORCE_STOP_HEATING, .reason = APP_STOP_REASON_UNDERVOLTAGE};
                (void)app_events_post(&ev, 0);
            }
        } else if (!st.heating.on || bus_voltage >= ((float)st.power.min_voltage + 0.5f)) {
            uv_stop_posted = false;
        }

        // Best-effort estimate of heater PCB resistance at 20°C (debug log only).
        // Assumes INA226 only measures the heater path.
        // R(T) is estimated from PWM duty and INA226 readings:
        //  - From current: R(T) ≈ duty * V / I_avg
        //  - From power:   R(T) ≈ duty * V^2 / P_avg
        // Back-calculate R20 via: R20 ≈ R(T) / (1 + alpha * (T - 20))
        // No strict gating: this is for debugging; expect more noise at low duty/current/power.
        const int64_t now_us = esp_timer_get_time();
        float duty_ratio = 0.0f;
        if (st.heating.pwm_max_duty > 0) {
            duty_ratio = (float)st.heating.pwm_current_duty / (float)st.heating.pwm_max_duty;
        }
        const float alpha = 0.00393f;
        const float denom = 1.0f + alpha * (st.temp.pt1000 - 20.0f);

        float r20_i = NAN;
        float r20_p = NAN;
        if (st.heating.on && st.heating.pwm_current_duty > 0 && bus_voltage > 0.5f && duty_ratio > 0.0f && isfinite(st.temp.pt1000) && denom > 0.1f) {
            if (current > 0.02f) {
                const float r_t_i = duty_ratio * bus_voltage / current;
                r20_i = r_t_i / denom;
            }
            if (power > 0.2f) {
                const float r_t_p = duty_ratio * bus_voltage * bus_voltage / power;
                r20_p = r_t_p / denom;
            }
        }

        if (isfinite(r20_i) && r20_i > 0.01f && r20_i < 50.0f) {
            if (!isfinite(r20_i_ewma_ohm)) r20_i_ewma_ohm = r20_i;
            else r20_i_ewma_ohm = 0.95f * r20_i_ewma_ohm + 0.05f * r20_i;
            r20_samples++;
        }
        if (isfinite(r20_p) && r20_p > 0.01f && r20_p < 50.0f) {
            if (!isfinite(r20_p_ewma_ohm)) r20_p_ewma_ohm = r20_p;
            else r20_p_ewma_ohm = 0.95f * r20_p_ewma_ohm + 0.05f * r20_p;
            r20_samples++;
        }

        if ((isfinite(r20_i) || isfinite(r20_p)) && (pcb_r20_last_log_us == 0 || (now_us - pcb_r20_last_log_us) >= 1000000)) {
            pcb_r20_last_log_us = now_us;
            ESP_LOGI(TAG,
                     "R20_i=%.3f(ewma=%.3f) R20_p=%.3f(ewma=%.3f) duty=%.3f V=%.2f I=%.2f P=%.2f T=%.1f samples=%u",
                     (double)r20_i,
                     (double)(isfinite(r20_i_ewma_ohm) ? r20_i_ewma_ohm : NAN),
                     (double)r20_p,
                     (double)(isfinite(r20_p_ewma_ohm) ? r20_p_ewma_ohm : NAN),
                     (double)duty_ratio,
                     (double)bus_voltage,
                     (double)current,
                     (double)power,
                     (double)st.temp.pt1000,
                     (unsigned)r20_samples);
        }

        // When heating is off, slow down to reduce I2C traffic.
        vTaskDelay(pdMS_TO_TICKS(st.heating.on ? 5 : 200));
    }
}

void app_service_power_meter_start(i2c_bus_handle_t i2c_bus) {
    s_i2c_bus = i2c_bus;
    xTaskCreate(ina226_task, "ina226", 4096, NULL, 3, NULL);
}
