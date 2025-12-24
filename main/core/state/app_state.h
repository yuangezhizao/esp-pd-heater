#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "pid_ctrl.h"

#define NVS_NAMESPACE "storage"                   // NVS 命名空间
#define NVS_KEY_PCB_RESISTANCE "pcbR"             // 铝基板 在 20℃ 时的阻值
#define NVS_KEY_SUPPLY_MAX_POWER "sMaxPwr"        // 电源最大功率限制
#define NVS_KEY_PID_EXPECT_TEMP "pExpTmp"         // PID 目标温度
#define NVS_KEY_PID_KP "pKp"                      // PID 比例系数
#define NVS_KEY_PID_KI "pKi"                      // PID 积分系数
#define NVS_KEY_PID_KD "pKd"                      // PID 微分系数
#define NVS_KEY_BUZZER_VOLUME "bVol"              // 蜂鸣器音量
#define NVS_KEY_BUZZER_NOTE "bNote"               // 蜂鸣器音符
#define NVS_KEY_BRIGHTNESS "br"                   // 屏幕亮度
#define NVS_KEY_UPPER_DIVIDER_RESISTANCE "uDivR"  // 上分压电阻
#define NVS_KEY_ADC_OFFSET "adcOffset"            // ADC 偏移校准值
#define NVS_KEY_TILT_THRESHOLD "tThres"           // 倾倒阈值角度
#define NVS_KEY_SHUNT_RESISTANCE "shuntR"         // INA226 采样电阻值(mΩ)
#define NVS_KEY_MIN_VOLTAGE "minVolt"             // 最低加热电压(V)

// 全局状态结构体
typedef struct {
    struct {
        bool on;                      // 加热开关状态
        uint32_t start_time;          // 加热开始时间(ms)
        bool is_temp_reached;         // 是否已达到目标温度
        uint16_t pcb_r_at_20c;        // 铝基板 在 20℃ 时的阻值
        uint8_t supply_max_power;     // 电源最大功率限制
        uint32_t pwm_max_duty;        // pwm 最大占空比
        uint32_t pwm_current_duty;    // pwm 当前占空比
    } heating;

    struct {
        float pt1000;                      // PT1000 温度
        float chip;                        // 芯片温度
        uint16_t pid_target;               // PID 目标温度
        pid_ctrl_parameter_t pid;          // PID 参数
        pid_ctrl_block_handle_t pid_ctrl;  // PID 控制器句柄
    } temp;

    struct {
        uint16_t upper_div_r;  // 上分压电阻
        int16_t offset;        // ADC 偏移校准值
        float voltage;         // ADC 电压值
    } adc;

    struct {
        float voltage;        // 电压
        float current;        // 电流
        float power;          // 功率
        float shunt_voltage;  // 分流电压
        uint8_t shunt_res;    // 采样电阻值(mΩ), 范围1-20
        uint8_t min_voltage;  // 最低加热电压(V), 范围4-20
    } power;

    struct {
        uint8_t brightness;     // 屏幕亮度
        uint8_t buzzer_volume;  // 蜂鸣器音量
        uint8_t buzzer_note;    // 蜂鸣器音符
    } config;

    struct {
        float tilt_angle;        // 倾斜角度
        uint8_t tilt_threshold;  // 倾倒阈值角度
    } lis2dh12;

    struct {
        bool nvs_need_save;        // 是否需要保存配置
        bool data_record_restart;  // 是否需要重新开始记录
    } system;
} app_state_t;

// 全局状态对象声明
extern app_state_t g_state;

// 并发：状态互斥锁与快照
void app_state_lock(void);
void app_state_unlock(void);
void app_state_snapshot(app_state_t *out);

// 核心功能函数
void app_state_init(void);
void app_state_load(void);  // 从NVS加载状态
void app_state_save(void);  // 保存状态到NVS

// 关键状态变更函数(只保留最重要的几个)
void app_state_set_heating(bool on);  // 设置加热状态
void app_state_update_pid(void);      // 更新PID参数

// 配置/参数设置（内部会标记需要保存到 NVS）
void app_state_set_pid_target(uint16_t target_temp);
void app_state_set_pid_kp(float kp);
void app_state_set_pid_ki(float ki);
void app_state_set_pid_kd(float kd);
void app_state_set_supply_max_power(uint8_t max_power_w);
void app_state_set_pcb_r_at_20c(uint16_t pcb_r_at_20c_x100);
void app_state_set_upper_div_r(uint16_t upper_div_r);
void app_state_set_adc_offset(int16_t offset_mv);
void app_state_set_brightness(uint8_t brightness_percent);
void app_state_set_buzzer_volume(uint8_t volume_percent);
void app_state_set_buzzer_note(uint8_t note);
void app_state_set_tilt_threshold(uint8_t threshold_deg);
void app_state_set_shunt_res(uint8_t shunt_res_mohm);  // 1-20
void app_state_set_min_voltage(uint8_t min_voltage_v); // 4-20
void app_state_set_data_record_restart(bool restart);
