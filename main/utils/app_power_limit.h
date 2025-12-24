#pragma once

#include <stdint.h>

float power_limit(float supply_voltage,
                  float max_power,
                  float resistance_at_20C,
                  float current_temperature,
                  uint32_t heating_start_time_ms,
                  float current_power_w,
                  uint32_t current_duty,
                  uint32_t max_duty);
