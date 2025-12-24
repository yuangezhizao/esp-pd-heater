#include "app_lcd_variant.h"

#include <stdbool.h>

#include "esp_log.h"
#include "nvs.h"

#define TAG "app_lcd_variant"

#define LCD_NVS_NAMESPACE "lcd"
#define LCD_NVS_KEY_VARIANT "variant"
#define LCD_NVS_KEY_LOCKED "locked"

static esp_err_t nvs_open_lcd(nvs_handle_t *out) {
    if (out == NULL) return ESP_ERR_INVALID_ARG;
    return nvs_open(LCD_NVS_NAMESPACE, NVS_READWRITE, out);
}

const char *app_lcd_variant_to_str(app_lcd_variant_t variant) {
    switch (variant) {
        case APP_LCD_VARIANT_HSD:
            return "HSD";
        case APP_LCD_VARIANT_BOE:
            return "BOE";
        default:
            return "UNKNOWN";
    }
}

esp_err_t app_lcd_variant_boot_select(app_lcd_variant_t *out_variant, bool *out_locked) {
    if (out_variant == NULL || out_locked == NULL) return ESP_ERR_INVALID_ARG;

    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open_lcd(&nvs);
    if (err != ESP_OK) return err;

    uint8_t variant_u8 = (uint8_t)APP_LCD_VARIANT_HSD;
    uint8_t locked_u8 = 0;

    esp_err_t err_variant = nvs_get_u8(nvs, LCD_NVS_KEY_VARIANT, &variant_u8);
    esp_err_t err_locked = nvs_get_u8(nvs, LCD_NVS_KEY_LOCKED, &locked_u8);

    bool is_first_boot = (err_variant == ESP_ERR_NVS_NOT_FOUND) || (err_locked == ESP_ERR_NVS_NOT_FOUND);
    if (is_first_boot) {
        variant_u8 = (uint8_t)APP_LCD_VARIANT_HSD;
        locked_u8 = 0;
        ESP_LOGI(TAG, "First boot: default LCD variant=%s", app_lcd_variant_to_str((app_lcd_variant_t)variant_u8));
    } else if (err_variant != ESP_OK) {
        nvs_close(nvs);
        return err_variant;
    } else if (err_locked != ESP_OK) {
        nvs_close(nvs);
        return err_locked;
    }

    if (locked_u8 == 0 && !is_first_boot) {
        variant_u8 = (variant_u8 == (uint8_t)APP_LCD_VARIANT_HSD) ? (uint8_t)APP_LCD_VARIANT_BOE : (uint8_t)APP_LCD_VARIANT_HSD;
        ESP_LOGI(TAG, "LCD not locked: toggled to variant=%s", app_lcd_variant_to_str((app_lcd_variant_t)variant_u8));
    } else {
        ESP_LOGI(TAG, "LCD %s: keep variant=%s",
                 locked_u8 ? "locked" : "unlocked",
                 app_lcd_variant_to_str((app_lcd_variant_t)variant_u8));
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvs, LCD_NVS_KEY_VARIANT, variant_u8));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvs, LCD_NVS_KEY_LOCKED, locked_u8));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_commit(nvs));
    nvs_close(nvs);

    *out_variant = (app_lcd_variant_t)variant_u8;
    *out_locked = (locked_u8 != 0);
    return ESP_OK;
}

esp_err_t app_lcd_variant_lock(void) {
    nvs_handle_t nvs = 0;
    esp_err_t err = nvs_open_lcd(&nvs);
    if (err != ESP_OK) return err;

    uint8_t locked_u8 = 0;
    esp_err_t err_locked = nvs_get_u8(nvs, LCD_NVS_KEY_LOCKED, &locked_u8);
    if (err_locked != ESP_OK && err_locked != ESP_ERR_NVS_NOT_FOUND) {
        nvs_close(nvs);
        return err_locked;
    }
    if (locked_u8 != 0) {
        nvs_close(nvs);
        return ESP_OK;
    }

    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvs, LCD_NVS_KEY_LOCKED, 1));
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_commit(nvs));
    nvs_close(nvs);

    ESP_LOGI(TAG, "LCD variant locked (cleared only by full NVS erase)");
    return ESP_OK;
}

