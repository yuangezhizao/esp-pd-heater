#pragma once

#include <stdint.h>

// 蜂鸣器音量计算
uint32_t calculate_buzzer_volume(uint8_t percent);

// 蜂鸣器启动任务
void buzzer_start_task(void *pvParameter);
