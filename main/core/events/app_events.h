#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"

typedef enum {
    APP_EVENT_SET_HEATING = 0,
    APP_EVENT_FORCE_STOP_HEATING,

    APP_EVENT_SET_PID_TARGET,
    APP_EVENT_SET_PID_KP,
    APP_EVENT_SET_PID_KI,
    APP_EVENT_SET_PID_KD,

    APP_EVENT_SET_SUPPLY_MAX_POWER,
    APP_EVENT_SET_PCB_R_AT_20C,
    APP_EVENT_SET_UPPER_DIV_R,
    APP_EVENT_SET_ADC_OFFSET,

    APP_EVENT_SET_BRIGHTNESS,
    APP_EVENT_SET_BUZZER_VOLUME,
    APP_EVENT_SET_BUZZER_NOTE,

    APP_EVENT_SET_TILT_THRESHOLD,
    APP_EVENT_SET_SHUNT_RES,
    APP_EVENT_SET_MIN_VOLTAGE,

    APP_EVENT_DATA_RECORD_RESTART,
} app_event_type_t;

typedef enum {
    APP_STOP_REASON_UNKNOWN = 0,
    APP_STOP_REASON_TILT,
    APP_STOP_REASON_OVERTEMP,
    APP_STOP_REASON_UNDERVOLTAGE,
} app_stop_reason_t;

typedef struct {
    app_event_type_t type;
    app_stop_reason_t reason;  // only for stop events
    union {
        bool b;
        uint8_t u8;
        uint16_t u16;
        int16_t i16;
        float f;
    } value;
} app_event_t;

void app_events_init(void);
bool app_events_post(const app_event_t *event, TickType_t ticks_to_wait);
bool app_events_receive(app_event_t *event, TickType_t ticks_to_wait);
