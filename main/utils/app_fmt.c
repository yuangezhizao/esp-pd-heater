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

    int num_width = (int)(width - unit_len);

    uint32_t abs_int = (uint32_t)fabsf(value);
    uint8_t integer_digits = count_u32_digits(abs_int) + (value < 0 ? 1 : 0);

    int decimal_places = num_width - (int)integer_digits - 1;  // -1 for '.'
    if (decimal_places < 0) decimal_places = 0;

    snprintf(result_str, result_str_len, "%0*.*f%s", num_width, decimal_places, (double)value, unit);
}

