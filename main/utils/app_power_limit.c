#include "app_power_limit.h"

#include <math.h>

#include "esp_timer.h"

float power_limit(float supply_voltage,
                  float max_power,
                  float resistance_at_20C,
                  float current_temperature,
                  uint32_t heating_start_time_ms,
                  float current_power_w,
                  uint32_t current_duty,
                  uint32_t max_duty,
                  uint8_t soft_start_time_s) {
    // Goal: keep INA226 measured power <= user max_power (conservative, no intentional overshoot).
    // NOTE: absolute "never exceed" isn't physically possible due to sensor/thermal latency;
    // this algorithm prioritizes conservatism and fast backoff when an over-limit sample is observed.

    static uint32_t s_last_start_time_ms = 0;
    static float s_last_max_power = 0.0f;
    static int64_t s_last_update_us = 0;

    static float s_p_full_est_w = 0.0f; // Estimated power at 100% duty (W)
    static float s_duty_cap = 0.0f;     // 0..1

    const int64_t now_us = esp_timer_get_time();
    float dt_s = 0.0f;
    if (s_last_update_us > 0 && now_us > s_last_update_us) {
        dt_s = (float)((double)(now_us - s_last_update_us) / 1000000.0);
    }
    s_last_update_us = now_us;
    // Avoid huge dt on the first call after a long idle, which would otherwise allow a large cap jump.
    if (dt_s > 0.20f) dt_s = 0.20f;

    if (heating_start_time_ms == 0) {
        // Heating off: reset session-local state.
        s_last_start_time_ms = 0;
        s_last_max_power = max_power;
        s_p_full_est_w = 0.0f;
        s_duty_cap = 0.0f;
        s_last_update_us = 0;
        return 0.0f;
    }
    if (heating_start_time_ms != s_last_start_time_ms) {
        // New heating session: start with the same conservative behavior as cold boot.
        s_last_start_time_ms = heating_start_time_ms;
        s_last_max_power = max_power;
        s_p_full_est_w = 0.0f;
        s_duty_cap = 0.0f;
        // Reset dt for this session.
        s_last_update_us = 0;
        dt_s = 0.0f;
    }

    if (!isfinite(max_power) || max_power <= 0.1f) return 1.0f;
    if (!isfinite(current_power_w) || current_power_w < 0.0f) current_power_w = 0.0f;
    if (!isfinite(supply_voltage) || supply_voltage < 0.0f) supply_voltage = 0.0f;

    // Safety margin: avoid intentionally sitting exactly at the limit.
    // Fixed guard so high-power settings don't get penalized by a percentage.
    const float guard_w = 0.1f;
    float effective_max_power_final = max_power - guard_w;
    if (effective_max_power_final < 0.0f) effective_max_power_final = 0.0f;

    // Time-based soft-start: ramp the *allowed max power* during the first seconds of a heating session.
    // This keeps the user-facing meaning intuitive: "soft_start_time_s seconds to reach max_power".
    float effective_max_power = effective_max_power_final;
    const float ramp_s = (float)soft_start_time_s;
    if (ramp_s > 0.0f) {
        const uint32_t now_ms = (uint32_t)(now_us / 1000);
        uint32_t elapsed_ms = 0;
        if (now_ms >= heating_start_time_ms) elapsed_ms = now_ms - heating_start_time_ms;
        const float ramp_ratio = fminf(fmaxf((float)elapsed_ms / (ramp_s * 1000.0f), 0.0f), 1.0f);
        effective_max_power = effective_max_power_final * ramp_ratio;
    }

    // Max power changed: scale down cap immediately if needed.
    if (s_last_max_power > 0.0f && max_power < s_last_max_power && s_p_full_est_w > 0.0f) {
        float cap_new = (max_power / s_last_max_power) * s_duty_cap;
        if (cap_new < s_duty_cap) s_duty_cap = cap_new;
    }
    s_last_max_power = max_power;

    float duty_ratio = 0.0f;
    if (max_duty > 0 && current_duty <= max_duty) {
        duty_ratio = (float)current_duty / (float)max_duty;
    }

    // Bootstrap: if we don't have a valid estimate yet, derive a conservative upper bound from V^2/R.
    // Keep factor modest to avoid a too-slow start, rely on fast backoff when an over-limit sample is observed.
    if (s_p_full_est_w <= 0.0f) {
        float p_full_upper = 0.0f;
        if (supply_voltage > 0.1f && isfinite(resistance_at_20C) && resistance_at_20C > 0.05f) {
            (void)current_temperature; // keep signature stable; temperature is not relied on for limiting.
            p_full_upper = (supply_voltage * supply_voltage) / resistance_at_20C;
            p_full_upper *= 1.15f; // guard for model error / transient.
        } else {
            // Fallback: assume the heater could be quite powerful at full duty.
            p_full_upper = 250.0f;
        }
        // Ensure p_full_upper not below final effective max power (avoid an overly pessimistic estimate).
        if (p_full_upper < effective_max_power_final) p_full_upper = effective_max_power_final;
        s_p_full_est_w = p_full_upper;
    }

    // Update full-power estimate from measurement when duty is meaningful.
    if (duty_ratio >= 0.02f && current_power_w >= 0.2f) {
        float p_full_sample = current_power_w / duty_ratio;
        if (isfinite(p_full_sample)) {
            // Clamp to a sane range to avoid noise blow-up.
            if (p_full_sample < effective_max_power_final) p_full_sample = effective_max_power_final;
            if (p_full_sample > 600.0f) p_full_sample = 600.0f;
            const float alpha = 0.12f;
            s_p_full_est_w = (1.0f - alpha) * s_p_full_est_w + alpha * p_full_sample;
        }
    }

    // Compute target cap from estimated full power.
    float cap_target = effective_max_power / s_p_full_est_w;
    cap_target = fminf(fmaxf(cap_target, 0.0f), 1.0f);

    // Fast backoff if we ever see an over-limit measurement.
    // Use the ramped max power for backoff threshold so soft-start is enforced even with sensing latency.
    float max_power_now = effective_max_power + guard_w;
    if (max_power_now < 0.0f) max_power_now = 0.0f;
    if (current_power_w > max_power_now && duty_ratio > 0.0f) {
        // Backoff reacts immediately when we observe an over-limit sample.
        float cap_backoff = duty_ratio * (max_power_now / current_power_w);
        cap_backoff = fminf(fmaxf(cap_backoff, 0.0f), 1.0f);
        if (cap_backoff < cap_target) cap_target = cap_backoff;
    }

    // Increase with headroom awareness (to avoid overshoot due to sensing latency), decrease immediately.
    float max_increase_per_s = 0.20f; // near limit
    if (current_power_w < 0.70f * effective_max_power) {
        max_increase_per_s = 0.60f;
    }
    if (current_power_w < 0.30f * effective_max_power) {
        max_increase_per_s = 1.00f;
    }
    // First iteration of a heating session: start small and ramp (avoid sudden jump on restart).
    if (s_duty_cap <= 0.0f) {
        s_duty_cap = fminf(cap_target, 0.05f);
        cap_target = s_duty_cap;
    }
    if (cap_target > s_duty_cap && dt_s > 0.0f) {
        float max_inc = max_increase_per_s * dt_s;
        if (cap_target > s_duty_cap + max_inc) cap_target = s_duty_cap + max_inc;
    }
    s_duty_cap = cap_target;

    return s_duty_cap;
}
