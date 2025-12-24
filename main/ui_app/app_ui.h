#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "lvgl.h"

// UI 组件句柄
extern lv_chart_series_t *ui_Chart1_series_1;
extern lv_chart_series_t *ui_Chart1_series_2;

// UI 初始化函数
void app_lvgl_display(void);
void lvgl_widgets_values_init(void);

// UI background tasks (render loop, chart updates, etc.)
void app_ui_render_start(void);
void app_ui_data_record_start(void);
