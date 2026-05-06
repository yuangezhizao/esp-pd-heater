#include "app_fmt.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static uint8_t count_u32_digits(uint32_t value) {
    uint8_t digits = 1;
    while (value >= 10) {
        value /= 10;
        digits++;
    }
    return digits;
}

void format_float_to_width(char *result_str, size_t result_str_len, uint8_t width, float value, const char *unit) {
    if (result_str == NULL || result_str_len == 0) return;
    if (unit == NULL) unit = "";

    size_t unit_len = strlen(unit);
    if (width <= unit_len) {
        snprintf(result_str, result_str_len, "%g%s", (double)value, unit);
        return;
    }

    if (!isfinite(value)) {
        int num_width = (int)(width - unit_len);
        if (num_width < 1) num_width = 1;
        snprintf(result_str, result_str_len, "%.*s%s", num_width, "-----", unit);
        return;
    }

    int num_width = (int)(width - unit_len);

    uint32_t abs_int = (uint32_t)fabsf(value);
    uint8_t integer_digits = count_u32_digits(abs_int) + (value < 0 ? 1 : 0);

    int decimal_places = num_width - (int)integer_digits - 1;  // -1 for '.'
    if (decimal_places < 0) decimal_places = 0;

    snprintf(result_str, result_str_len, "%0*.*f%s", num_width, decimal_places, (double)value, unit);
}

static void pad_numeric_to_width_5(char *dst, size_t dst_len, const char *src) {
    size_t len = strlen(src);
    if (len >= 5) {
        snprintf(dst, dst_len, "%s", src);
        return;
    }

    if (src[0] == '-') {
        char tmp[16];
        size_t body_len = len - 1;
        size_t zeros = 5 - len;
        tmp[0] = '-';
        memset(tmp + 1, '0', zeros);
        memcpy(tmp + 1 + zeros, src + 1, body_len + 1);
        snprintf(dst, dst_len, "%s", tmp);
        return;
    }

    char tmp[16];
    size_t zeros = 5 - len;
    memset(tmp, '0', zeros);
    memcpy(tmp + zeros, src, len + 1);
    snprintf(dst, dst_len, "%s", tmp);
}

void format_float_to_5chars(char *result_str, size_t result_str_len, float value, const char *unit) {
    if (result_str == NULL || result_str_len == 0) return;
    if (unit == NULL) unit = "";
    if (!isfinite(value)) {
        snprintf(result_str, result_str_len, "--.--%s", unit);
        return;
    }

    char number_buf[16];
    for (int decimals = 2; decimals >= 0; decimals--) {
        snprintf(number_buf, sizeof(number_buf), "%.*f", decimals, (double)value);
        if (strlen(number_buf) <= 5) {
            char padded[16];
            pad_numeric_to_width_5(padded, sizeof(padded), number_buf);
            snprintf(result_str, result_str_len, "%s%s", padded, unit);
            return;
        }
    }

    // Fall back to rounded integer and clip to the rightmost 5 characters if still too wide.
    snprintf(number_buf, sizeof(number_buf), "%.0f", (double)value);
    if (strlen(number_buf) > 5) {
        snprintf(result_str, result_str_len, "%s%s", number_buf + strlen(number_buf) - 5, unit);
    } else {
        char padded[16];
        pad_numeric_to_width_5(padded, sizeof(padded), number_buf);
        snprintf(result_str, result_str_len, "%s%s", padded, unit);
    }
}
