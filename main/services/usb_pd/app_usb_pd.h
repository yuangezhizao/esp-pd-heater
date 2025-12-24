#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "i2c_bus.h"

// PD芯片类型枚举
typedef enum {
    PD_CHIP_HUSB238,
    PD_CHIP_CH32X035
} pd_chip_type_t;

// PD电压枚举
typedef enum {
    PD_VOLT_5V,
    PD_VOLT_9V,
    PD_VOLT_12V,
    PD_VOLT_15V,
    PD_VOLT_18V,
    PD_VOLT_20V,
    PD_VOLT_MAX
} pd_voltage_t;

// 初始化PD
void app_pd_init(i2c_bus_handle_t i2c_bus);

// 更新PD状态
void app_pd_update(void);

// 请求电压
bool app_pd_request_voltage(pd_voltage_t voltage);

// 请求最大电压
void app_pd_request_max_voltage(void);

// 线程安全地拷贝状态信息文本（始终保证 NUL 结尾）
bool app_pd_get_info_text_copy(char *out, size_t out_len);
