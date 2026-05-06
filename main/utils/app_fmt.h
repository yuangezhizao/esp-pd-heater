#pragma once

#include <stddef.h>
#include <stdint.h>

void format_float_to_width(char *result_str, size_t result_str_len, uint8_t width, float value, const char *unit);
void format_float_to_5chars(char *result_str, size_t result_str_len, float value, const char *unit);
