#pragma once

#include <stdbool.h>
#include <stdint.h>

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

// PD状态结构体
typedef struct {
    pd_chip_type_t chip_type;  // 芯片类型
    bool is_attached;          // 是否连接
    bool is_cc2;               // 是否CC2连接
    pd_voltage_t max_voltage;  // 最大支持电压
    char info_text[512];       // 状态信息文本
    void* chip_handle;         // 芯片句柄(husb238或ch32x035)
} pd_state_t;

// 初始化PD
void app_pd_init(i2c_bus_handle_t i2c_bus);

// 更新PD状态
void app_pd_update(void);

// 请求电压
bool app_pd_request_voltage(pd_voltage_t voltage);

// 请求最大电压
void app_pd_request_max_voltage(void);

// 获取状态信息文本
const char* app_pd_get_info_text(void);
