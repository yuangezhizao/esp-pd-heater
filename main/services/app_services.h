#pragma once

#include "i2c_bus.h"

void app_service_heater_pwm_start(void);
void app_service_usb_pd_start(i2c_bus_handle_t i2c_bus);
void app_service_power_meter_start(i2c_bus_handle_t i2c_bus);
void app_service_temp_control_start(void);
void app_service_motion_start(i2c_bus_handle_t i2c_bus);
void app_service_chip_temp_start(void);
void app_service_backlight_start(void);

