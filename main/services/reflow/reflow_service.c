#include "reflow_service.h"

#include <math.h>
#include <string.h>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

#define REFLOW_PROFILE_COUNT 3
#define REFLOW_POINT_COUNT 8

#define REFLOW_OVERTEMP_C 300

typedef struct {
    reflow_state_t state;
    uint8_t profile_id;

    // Time bookkeeping (ms)
    uint32_t base_elapsed_ms;  // accumulated time while previously running
    uint32_t run_start_ms;     // timestamp when current RUNNING segment started

    float tset_c;
    uint32_t revision[REFLOW_PROFILE_COUNT];
    uint32_t run_id;
} reflow_state_internal_t;

static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static reflow_state_internal_t s = {0};

// Compile-time defaults. Peak is 220C by design; last point cools down the preview.
static const reflow_point_t s_profile_defaults[REFLOW_PROFILE_COUNT][REFLOW_POINT_COUNT] = {
    // REFLOW1 (slow)
    {
        {0, 25}, {60, 120}, {150, 160}, {210, 160}, {270, 200}, {330, 220}, {360, 220}, {420, 50},
    },
    // REFLOW2 (medium)
    {
        {0, 25}, {45, 120}, {120, 150}, {180, 170}, {240, 210}, {270, 220}, {300, 220}, {360, 50},
    },
    // REFLOW3 (fast)
    {
        {0, 25}, {30, 120}, {90, 160}, {120, 170}, {180, 220}, {210, 220}, {240, 220}, {300, 50},
    },
};

static reflow_point_t s_profile_points[REFLOW_PROFILE_COUNT][REFLOW_POINT_COUNT];

static uint16_t profile_total_s_unsafe(uint8_t profile_id) {
    return s_profile_points[profile_id][REFLOW_POINT_COUNT - 1].t_s;
}

static float interpolate_tset_unsafe(uint8_t profile_id, uint32_t elapsed_ms) {
    const reflow_point_t *pts = s_profile_points[profile_id];
    const float t_s = (float)elapsed_ms / 1000.0f;

    if (t_s <= (float)pts[0].t_s) return (float)pts[0].temp_c;

    const uint16_t total_s = pts[REFLOW_POINT_COUNT - 1].t_s;
    if (t_s >= (float)total_s) return (float)pts[REFLOW_POINT_COUNT - 1].temp_c;

    // REFLOW_POINT_COUNT is tiny, linear scan is fine.
    for (size_t i = 0; i + 1 < REFLOW_POINT_COUNT; i++) {
        const float t0 = (float)pts[i].t_s;
        const float t1 = (float)pts[i + 1].t_s;
        if (t_s >= t0 && t_s <= t1) {
            const float y0 = (float)pts[i].temp_c;
            const float y1 = (float)pts[i + 1].temp_c;
            const float dt = t1 - t0;
            if (dt <= 0.0f) return y1;
            const float a = (t_s - t0) / dt;
            return y0 + a * (y1 - y0);
        }
    }
    return (float)pts[REFLOW_POINT_COUNT - 1].temp_c;
}

static bool validate_point_edit_unsafe(uint8_t profile_id, uint8_t point_idx, uint16_t t_s) {
    if (profile_id >= REFLOW_PROFILE_COUNT) return false;
    if (point_idx >= REFLOW_POINT_COUNT) return false;

    if (point_idx > 0) {
        const uint16_t t_prev = s_profile_points[profile_id][point_idx - 1].t_s;
        if (t_s <= t_prev) return false;
    }
    if (point_idx + 1 < REFLOW_POINT_COUNT) {
        const uint16_t t_next = s_profile_points[profile_id][point_idx + 1].t_s;
        if (t_s >= t_next) return false;
    }
    return true;
}

void reflow_service_init(void) {
    portENTER_CRITICAL(&s_lock);
    memset(&s, 0, sizeof(s));
    s.state = REFLOW_STATE_IDLE;
    s.profile_id = 0;
    s.base_elapsed_ms = 0;
    s.run_start_ms = 0;
    s.tset_c = (float)s_profile_defaults[0][0].temp_c;
    s.run_id = 0;
    memset(s.revision, 0, sizeof(s.revision));
    memcpy(s_profile_points, s_profile_defaults, sizeof(s_profile_points));
    portEXIT_CRITICAL(&s_lock);
}

void reflow_service_set_profile(uint8_t profile_id) {
    if (profile_id >= REFLOW_PROFILE_COUNT) return;
    portENTER_CRITICAL(&s_lock);
    if (s.state == REFLOW_STATE_RUNNING) {
        portEXIT_CRITICAL(&s_lock);
        return;
    }
    s.profile_id = profile_id;
    // Keep PAUSED elapsed; otherwise show profile start.
    if (s.state != REFLOW_STATE_PAUSED) {
        s.base_elapsed_ms = 0;
    }
    s.run_start_ms = 0;
    s.tset_c = interpolate_tset_unsafe(s.profile_id, s.base_elapsed_ms);
    portEXIT_CRITICAL(&s_lock);
}

void reflow_service_start(uint32_t now_ms) {
    portENTER_CRITICAL(&s_lock);
    if (s.state == REFLOW_STATE_IDLE) {
        s.base_elapsed_ms = 0;
        s.run_start_ms = now_ms;
        s.tset_c = interpolate_tset_unsafe(s.profile_id, 0);
        s.run_id++;
        s.state = REFLOW_STATE_RUNNING;
    } else if (s.state == REFLOW_STATE_PAUSED) {
        s.run_start_ms = now_ms;
        s.state = REFLOW_STATE_RUNNING;
    }
    portEXIT_CRITICAL(&s_lock);
}

void reflow_service_pause(uint32_t now_ms) {
    portENTER_CRITICAL(&s_lock);
    if (s.state == REFLOW_STATE_RUNNING) {
        const uint32_t elapsed_ms = s.base_elapsed_ms + (now_ms - s.run_start_ms);
        s.base_elapsed_ms = elapsed_ms;
        s.run_start_ms = 0;
        s.tset_c = interpolate_tset_unsafe(s.profile_id, s.base_elapsed_ms);
        s.state = REFLOW_STATE_PAUSED;
    }
    portEXIT_CRITICAL(&s_lock);
}

void reflow_service_stop(void) {
    portENTER_CRITICAL(&s_lock);
    s.state = REFLOW_STATE_IDLE;
    s.base_elapsed_ms = 0;
    s.run_start_ms = 0;
    s.tset_c = interpolate_tset_unsafe(s.profile_id, 0);
    portEXIT_CRITICAL(&s_lock);
}

bool reflow_service_edit_point(uint8_t profile_id, uint8_t point_idx, uint16_t t_s, int16_t temp_c) {
    if (profile_id >= REFLOW_PROFILE_COUNT || point_idx >= REFLOW_POINT_COUNT) return false;

    // Keep temp edits within a reasonable range. Hard overtemp stop is separate (300C).
    if (temp_c < 20) temp_c = 20;
    if (temp_c > 260) temp_c = 260;

    bool ok = false;
    portENTER_CRITICAL(&s_lock);
    if (s.state != REFLOW_STATE_RUNNING) {
        ok = validate_point_edit_unsafe(profile_id, point_idx, t_s);
        if (ok) {
            s_profile_points[profile_id][point_idx].t_s = t_s;
            s_profile_points[profile_id][point_idx].temp_c = temp_c;
            s.revision[profile_id]++;
            if (profile_id == s.profile_id) {
                const uint32_t elapsed_ms = (s.state == REFLOW_STATE_PAUSED) ? s.base_elapsed_ms : 0;
                s.tset_c = interpolate_tset_unsafe(s.profile_id, elapsed_ms);
            }
        }
    }
    portEXIT_CRITICAL(&s_lock);
    return ok;
}

void reflow_service_reset_profile(uint8_t profile_id) {
    if (profile_id >= REFLOW_PROFILE_COUNT) return;
    portENTER_CRITICAL(&s_lock);
    if (s.state != REFLOW_STATE_RUNNING) {
        memcpy(&s_profile_points[profile_id][0], &s_profile_defaults[profile_id][0], sizeof(s_profile_points[0]));
        s.revision[profile_id]++;
        if (profile_id == s.profile_id) {
            const uint32_t elapsed_ms = (s.state == REFLOW_STATE_PAUSED) ? s.base_elapsed_ms : 0;
            s.tset_c = interpolate_tset_unsafe(s.profile_id, elapsed_ms);
        }
    }
    portEXIT_CRITICAL(&s_lock);
}

bool reflow_service_get_profile_points(uint8_t profile_id, reflow_point_t *out_points, size_t point_count) {
    if (out_points == NULL) return false;
    if (profile_id >= REFLOW_PROFILE_COUNT) return false;
    if (point_count < REFLOW_POINT_COUNT) return false;
    portENTER_CRITICAL(&s_lock);
    memcpy(out_points, s_profile_points[profile_id], sizeof(s_profile_points[profile_id]));
    portEXIT_CRITICAL(&s_lock);
    return true;
}

reflow_state_t reflow_service_tick(uint32_t now_ms, float measured_temp_c, float *out_tset_c, bool *out_stopped) {
    if (out_tset_c) *out_tset_c = NAN;
    if (out_stopped) *out_stopped = false;

    portENTER_CRITICAL(&s_lock);

    if (s.state == REFLOW_STATE_RUNNING) {
        const uint32_t elapsed_ms = s.base_elapsed_ms + (now_ms - s.run_start_ms);
        s.tset_c = interpolate_tset_unsafe(s.profile_id, elapsed_ms);

        // Hard overtemp: stop immediately and return to IDLE (user requested no FAULT latch).
        if (isfinite(measured_temp_c) && measured_temp_c >= (float)REFLOW_OVERTEMP_C) {
            s.state = REFLOW_STATE_IDLE;
            s.base_elapsed_ms = 0;
            s.run_start_ms = 0;
            s.tset_c = interpolate_tset_unsafe(s.profile_id, 0);
            if (out_stopped) *out_stopped = true;
        } else {
            const uint32_t total_ms = (uint32_t)profile_total_s_unsafe(s.profile_id) * 1000U;
            if (elapsed_ms >= total_ms) {
                s.state = REFLOW_STATE_IDLE;
                s.base_elapsed_ms = 0;
                s.run_start_ms = 0;
                s.tset_c = interpolate_tset_unsafe(s.profile_id, 0);
                if (out_stopped) *out_stopped = true;
            }
        }
    }

    if (out_tset_c) *out_tset_c = s.tset_c;
    const reflow_state_t state = s.state;

    portEXIT_CRITICAL(&s_lock);
    return state;
}

void reflow_service_snapshot(reflow_snapshot_t *out) {
    if (out == NULL) return;
    const uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    portENTER_CRITICAL(&s_lock);
    out->state = s.state;
    out->profile_id = s.profile_id;

    uint32_t elapsed_ms = s.base_elapsed_ms;
    if (s.state == REFLOW_STATE_RUNNING) elapsed_ms += (now_ms - s.run_start_ms);
    out->elapsed_s = (uint16_t)(elapsed_ms / 1000U);
    out->total_s = profile_total_s_unsafe(s.profile_id);
    out->tset_c = s.tset_c;
    out->profile_revision = s.revision[s.profile_id];
    out->run_id = s.run_id;
    portEXIT_CRITICAL(&s_lock);
}
