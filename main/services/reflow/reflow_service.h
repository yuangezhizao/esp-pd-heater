#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    REFLOW_STATE_IDLE = 0,
    REFLOW_STATE_RUNNING,
    REFLOW_STATE_PAUSED,
} reflow_state_t;

typedef struct {
    uint16_t t_s;     // seconds since profile start
    int16_t temp_c;   // target temperature in Celsius
} reflow_point_t;

typedef struct {
    reflow_state_t state;
    uint8_t profile_id;        // 0..2
    uint16_t elapsed_s;
    uint16_t total_s;
    float tset_c;
    uint32_t profile_revision; // increments when selected profile points change
    uint32_t run_id;           // increments on each start (used by UI to reset temp chart)
} reflow_snapshot_t;

// Initialize runtime state and load default profiles into editable RAM copies.
void reflow_service_init(void);

// Select active profile (0..2). Ignored while running.
void reflow_service_set_profile(uint8_t profile_id);

// Start from IDLE (reset elapsed) or resume from PAUSED.
void reflow_service_start(uint32_t now_ms);

// Pause (freeze time) and keep last tset. Ignored unless RUNNING.
void reflow_service_pause(uint32_t now_ms);

// Stop and reset to IDLE.
void reflow_service_stop(void);

// Edit a point in the selected profile (RAM only). Returns false on validation failure.
bool reflow_service_edit_point(uint8_t profile_id, uint8_t point_idx, uint16_t t_s, int16_t temp_c);

// Reset a profile's editable points back to defaults.
void reflow_service_reset_profile(uint8_t profile_id);

// Replace all editable points for a profile at once (RAM only).
// Returns false if validation fails or while RUNNING.
bool reflow_service_replace_profile_points(uint8_t profile_id, const reflow_point_t *points, size_t point_count);

// Copy current editable points for a profile into caller-provided buffer.
bool reflow_service_get_profile_points(uint8_t profile_id, reflow_point_t *out_points, size_t point_count);

// Periodic update. Computes Tset for RUNNING and stops to IDLE if:
// - elapsed reaches end of profile, or
// - measured temperature >= 300C (hard overtemp)
// Returns current state after applying stop conditions.
reflow_state_t reflow_service_tick(uint32_t now_ms, float measured_temp_c, float *out_tset_c, bool *out_stopped);

// Snapshot for UI/diagnostics (thread-safe).
void reflow_service_snapshot(reflow_snapshot_t *out);

// Persist edited profiles / selected profile if they changed.
void reflow_service_save_if_dirty(void);

#ifdef __cplusplus
}
#endif
