#pragma once

#include <stdbool.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_LCD_VARIANT_HSD = 0,
    APP_LCD_VARIANT_BOE = 1,
} app_lcd_variant_t;

/**
 * @brief Select LCD variant on boot.
 *
 * Rules:
 * - First boot (no NVS keys): default HSD, unlocked.
 * - If not locked: toggle variant on each boot and persist immediately.
 * - If locked: keep persisted variant.
 *
 * @param[out] out_variant Selected variant for this boot
 * @param[out] out_locked  Current lock state
 */
esp_err_t app_lcd_variant_boot_select(app_lcd_variant_t *out_variant, bool *out_locked);

/**
 * @brief Lock the current LCD selection (persistent). Only cleared by full NVS erase.
 */
esp_err_t app_lcd_variant_lock(void);

const char *app_lcd_variant_to_str(app_lcd_variant_t variant);

#ifdef __cplusplus
}
#endif
