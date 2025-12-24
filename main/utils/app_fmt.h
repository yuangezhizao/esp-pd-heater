#pragma once

#include <stddef.h>
#include <stdint.h>

void format_float_to_width(char *result_str, size_t result_str_len, uint8_t width, float value, const char *unit);

