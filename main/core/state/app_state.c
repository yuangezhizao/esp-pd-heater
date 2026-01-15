#include "app_state.h"

#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"

#define TAG "app_state"

// 全局状态对象定义
app_state_t g_state = {
    .heating.pwm_max_duty = 1023,
};

static SemaphoreHandle_t state_mutex = NULL;

void app_state_lock(void) {
    if (state_mutex) {
        xSemaphoreTake(state_mutex, portMAX_DELAY);
    }
}

void app_state_unlock(void) {
    if (state_mutex) {
        xSemaphoreGive(state_mutex);
    }
}

void app_state_snapshot(app_state_t *out) {
    if (out == NULL) return;
    app_state_lock();
    *out = g_state;
    app_state_unlock();
}

void app_state_init(void) {
    if (state_mutex == NULL) {
        state_mutex = xSemaphoreCreateMutex();
    }

    app_state_lock();
    // 加热相关初始化
    g_state.heating.on = false;
    g_state.heating.pwm_current_duty = 0;
    g_state.heating.start_time = 0;
    g_state.heating.is_temp_reached = false;
    g_state.heating.pcb_r_at_20c = 180;
    g_state.heating.supply_max_power = 5;
    g_state.heating.soft_start_time_s = 3;

    // 温度相关初始化
    g_state.temp.pt1000 = -99.0f;
    g_state.temp.chip = -99.0f;
    g_state.temp.pid_target = 45;
    g_state.temp.pid.kp = 20;
    g_state.temp.pid.ki = 0.16f;
    g_state.temp.pid.kd = 20;
    g_state.temp.pid.max_output = 0;
    g_state.temp.pid.min_output = 0;
    g_state.temp.pid.max_integral = 1000;
    g_state.temp.pid.min_integral = -1000;
    g_state.temp.pid.cal_type = PID_CAL_TYPE_POSITIONAL;

    // ADC相关初始化
    g_state.adc.upper_div_r = 3000;
    g_state.adc.offset = 0;

    // 设备配置初始化
    g_state.config.brightness = 80;
    g_state.config.buzzer_volume = 100;
    g_state.config.buzzer_note = 86;

    // 系统状态初始化
    g_state.system.nvs_need_save = false;
    g_state.system.data_record_restart = false;

    // lis2dh12 倾斜角度初始化
    g_state.lis2dh12.tilt_angle = 0.0f;
    g_state.lis2dh12.tilt_threshold = 135;

    // 电源相关初始化
    g_state.power.voltage = 0;
    g_state.power.current = 0;
    g_state.power.power = 0;
    g_state.power.shunt_voltage = 0;
    g_state.power.shunt_res = 5;    // 默认5mΩ
    g_state.power.min_voltage = 6;  // 默认6V

    app_state_unlock();
    ESP_LOGI(TAG, "App state initialized");
}

void app_state_set_heating(bool on) {
    app_state_lock();
    if (g_state.heating.on != on) {
        g_state.heating.on = on;
        g_state.system.data_record_restart = on;
        if (on) {
            g_state.heating.start_time = esp_timer_get_time() / 1000;
            g_state.heating.is_temp_reached = false;
        } else {
            g_state.heating.start_time = 0;
            g_state.heating.pwm_current_duty = 0;
        }
        ESP_LOGI(TAG, "Heating state changed to: %d", on);
    }
    app_state_unlock();
}

void app_state_update_pid(void) {
    app_state_lock();
    pid_update_parameters(g_state.temp.pid_ctrl, &g_state.temp.pid);
    app_state_unlock();
}

static void mark_nvs_dirty_if_changed(bool changed) {
    if (changed) g_state.system.nvs_need_save = true;
}

void app_state_set_pid_target(uint16_t target_temp) {
    app_state_lock();
    bool changed = (g_state.temp.pid_target != target_temp);
    g_state.temp.pid_target = target_temp;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_pid_kp(float kp) {
    app_state_lock();
    bool changed = (g_state.temp.pid.kp != kp);
    g_state.temp.pid.kp = kp;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_pid_ki(float ki) {
    app_state_lock();
    bool changed = (g_state.temp.pid.ki != ki);
    g_state.temp.pid.ki = ki;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_pid_kd(float kd) {
    app_state_lock();
    bool changed = (g_state.temp.pid.kd != kd);
    g_state.temp.pid.kd = kd;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_supply_max_power(uint8_t max_power_w) {
    app_state_lock();
    bool changed = (g_state.heating.supply_max_power != max_power_w);
    g_state.heating.supply_max_power = max_power_w;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_pcb_r_at_20c(uint16_t pcb_r_at_20c_x100) {
    app_state_lock();
    bool changed = (g_state.heating.pcb_r_at_20c != pcb_r_at_20c_x100);
    g_state.heating.pcb_r_at_20c = pcb_r_at_20c_x100;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_upper_div_r(uint16_t upper_div_r) {
    app_state_lock();
    bool changed = (g_state.adc.upper_div_r != upper_div_r);
    g_state.adc.upper_div_r = upper_div_r;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_adc_offset(int16_t offset_mv) {
    app_state_lock();
    bool changed = (g_state.adc.offset != offset_mv);
    g_state.adc.offset = offset_mv;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_brightness(uint8_t brightness_percent) {
    app_state_lock();
    bool changed = (g_state.config.brightness != brightness_percent);
    g_state.config.brightness = brightness_percent;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_buzzer_volume(uint8_t volume_percent) {
    app_state_lock();
    bool changed = (g_state.config.buzzer_volume != volume_percent);
    g_state.config.buzzer_volume = volume_percent;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_buzzer_note(uint8_t note) {
    app_state_lock();
    bool changed = (g_state.config.buzzer_note != note);
    g_state.config.buzzer_note = note;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_tilt_threshold(uint8_t threshold_deg) {
    app_state_lock();
    bool changed = (g_state.lis2dh12.tilt_threshold != threshold_deg);
    g_state.lis2dh12.tilt_threshold = threshold_deg;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_shunt_res(uint8_t shunt_res_mohm) {
    app_state_lock();
    // 限制范围在1-20mΩ
    if (shunt_res_mohm < 1) shunt_res_mohm = 1;
    if (shunt_res_mohm > 20) shunt_res_mohm = 20;
    bool changed = (g_state.power.shunt_res != shunt_res_mohm);
    g_state.power.shunt_res = shunt_res_mohm;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_min_voltage(uint8_t min_voltage_v) {
    app_state_lock();
    // 限制范围在4-20V
    if (min_voltage_v < 4) min_voltage_v = 4;
    if (min_voltage_v > 20) min_voltage_v = 20;
    bool changed = (g_state.power.min_voltage != min_voltage_v);
    g_state.power.min_voltage = min_voltage_v;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_soft_start_time(uint8_t soft_start_time_s) {
    app_state_lock();
    if (soft_start_time_s > 100) soft_start_time_s = 100;
    bool changed = (g_state.heating.soft_start_time_s != soft_start_time_s);
    g_state.heating.soft_start_time_s = soft_start_time_s;
    mark_nvs_dirty_if_changed(changed);
    app_state_unlock();
}

void app_state_set_data_record_restart(bool restart) {
    app_state_lock();
    g_state.system.data_record_restart = restart;
    app_state_unlock();
}

// NVS存取相关函数
void app_state_load(void) {
    app_state_lock();
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);

    if (err == ESP_OK) {
        uint16_t value_u16;
        uint8_t value_u8;
        int16_t value_i16;

        if (nvs_get_u16(nvs_handle, NVS_KEY_PID_KP, &value_u16) == ESP_OK) {
            g_state.temp.pid.kp = (float)value_u16;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_PID_KP not found, using default value: %.1f", g_state.temp.pid.kp);
        }

        if (nvs_get_u16(nvs_handle, NVS_KEY_PID_KI, &value_u16) == ESP_OK) {
            // Store Ki in NVS as Ki_x1000 to preserve fractional UI adjustments.
            g_state.temp.pid.ki = (float)value_u16 / 1000.0f;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_PID_KI not found, using default value: %.1f", g_state.temp.pid.ki);
        }

        if (nvs_get_u16(nvs_handle, NVS_KEY_PID_KD, &value_u16) == ESP_OK) {
            g_state.temp.pid.kd = (float)value_u16;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_PID_KD not found, using default value: %.1f", g_state.temp.pid.kd);
        }

        if (nvs_get_u16(nvs_handle, NVS_KEY_PCB_RESISTANCE, &value_u16) == ESP_OK) {
            g_state.heating.pcb_r_at_20c = value_u16;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_PCB_RESISTANCE not found, using default value: %d", g_state.heating.pcb_r_at_20c);
        }

        if (nvs_get_u8(nvs_handle, NVS_KEY_SUPPLY_MAX_POWER, &value_u8) == ESP_OK) {
            g_state.heating.supply_max_power = value_u8;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_SUPPLY_MAX_POWER not found, using default value: %d", g_state.heating.supply_max_power);
        }

        if (nvs_get_u16(nvs_handle, NVS_KEY_PID_EXPECT_TEMP, &value_u16) == ESP_OK) {
            g_state.temp.pid_target = value_u16;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_PID_EXPECT_TEMP not found, using default value: %d", g_state.temp.pid_target);
        }

        if (nvs_get_u8(nvs_handle, NVS_KEY_BUZZER_VOLUME, &value_u8) == ESP_OK) {
            g_state.config.buzzer_volume = value_u8;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_BUZZER_VOLUME not found, using default value: %d", g_state.config.buzzer_volume);
        }

        if (nvs_get_u8(nvs_handle, NVS_KEY_BUZZER_NOTE, &value_u8) == ESP_OK) {
            g_state.config.buzzer_note = value_u8;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_BUZZER_NOTE not found, using default value: %d", g_state.config.buzzer_note);
        }

        if (nvs_get_u8(nvs_handle, NVS_KEY_BRIGHTNESS, &value_u8) == ESP_OK) {
            g_state.config.brightness = value_u8;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_BRIGHTNESS not found, using default value: %d", g_state.config.brightness);
        }

        if (nvs_get_u16(nvs_handle, NVS_KEY_UPPER_DIVIDER_RESISTANCE, &value_u16) == ESP_OK) {
            g_state.adc.upper_div_r = value_u16;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_UPPER_DIVIDER_RESISTANCE not found, using default value: %d", g_state.adc.upper_div_r);
        }

        if (nvs_get_i16(nvs_handle, NVS_KEY_ADC_OFFSET, &value_i16) == ESP_OK) {
            g_state.adc.offset = value_i16;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_ADC_OFFSET not found, using default value: %d", g_state.adc.offset);
        }

        if (nvs_get_u8(nvs_handle, NVS_KEY_TILT_THRESHOLD, &value_u8) == ESP_OK) {
            g_state.lis2dh12.tilt_threshold = value_u8;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_TILT_THRESHOLD not found, using default value: %d", g_state.lis2dh12.tilt_threshold);
        }

        if (nvs_get_u8(nvs_handle, NVS_KEY_SHUNT_RESISTANCE, &value_u8) == ESP_OK) {
            // 限制范围在1-20mΩ
            if (value_u8 >= 1 && value_u8 <= 20) {
                g_state.power.shunt_res = value_u8;
            } else {
                g_state.power.shunt_res = 5;  // 超出范围使用默认值
                ESP_LOGW(TAG, "Invalid shunt resistance value: %d, using default", value_u8);
            }
        } else {
            ESP_LOGW(TAG, "NVS_KEY_SHUNT_RESISTANCE not found, using default value: %d", g_state.power.shunt_res);
        }

        if (nvs_get_u8(nvs_handle, NVS_KEY_MIN_VOLTAGE, &value_u8) == ESP_OK) {
            g_state.power.min_voltage = value_u8;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_MIN_VOLTAGE not found, using default value: %d", g_state.power.min_voltage);
        }

        if (nvs_get_u8(nvs_handle, NVS_KEY_SOFT_START_TIME, &value_u8) == ESP_OK) {
            g_state.heating.soft_start_time_s = value_u8;
        } else {
            ESP_LOGW(TAG, "NVS_KEY_SOFT_START_TIME not found, using default value: %d", g_state.heating.soft_start_time_s);
        }

    } else {
        ESP_LOGE(TAG, "Failed to open NVS for loading variables");
    }

    nvs_close(nvs_handle);
    app_state_unlock();
}

void app_state_save(void) {
    app_state_lock();
    if (!g_state.system.nvs_need_save) {
        app_state_unlock();
        return;
    }

    // Optimistically clear dirty flag under lock. Any changes during NVS I/O will set it again.
    g_state.system.nvs_need_save = false;

    // Copy values under lock, then do NVS I/O without holding the mutex.
    float pid_kp = g_state.temp.pid.kp;
    float pid_ki = g_state.temp.pid.ki;
    float pid_kd = g_state.temp.pid.kd;
    uint16_t pcb_r_at_20c = g_state.heating.pcb_r_at_20c;
    uint8_t supply_max_power = g_state.heating.supply_max_power;
    uint16_t pid_target = g_state.temp.pid_target;
    uint8_t buzzer_volume = g_state.config.buzzer_volume;
    uint8_t buzzer_note = g_state.config.buzzer_note;
    uint8_t brightness = g_state.config.brightness;
    uint16_t upper_div_r = g_state.adc.upper_div_r;
    int16_t adc_offset = g_state.adc.offset;
    uint8_t tilt_threshold = g_state.lis2dh12.tilt_threshold;
    uint8_t shunt_res = g_state.power.shunt_res;
    uint8_t min_voltage = g_state.power.min_voltage;
    uint8_t soft_start_time_s = g_state.heating.soft_start_time_s;
    app_state_unlock();

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);

    bool saved = false;
    if (err == ESP_OK) {
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u16(nvs_handle, NVS_KEY_PID_KP, (uint16_t)pid_kp));
        // Store Ki in NVS as Ki_x1000 to preserve fractional UI adjustments.
        uint16_t pid_ki_x1000 = (uint16_t)lroundf(pid_ki * 1000.0f);
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u16(nvs_handle, NVS_KEY_PID_KI, pid_ki_x1000));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u16(nvs_handle, NVS_KEY_PID_KD, (uint16_t)pid_kd));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u16(nvs_handle, NVS_KEY_PCB_RESISTANCE, pcb_r_at_20c));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvs_handle, NVS_KEY_SUPPLY_MAX_POWER, supply_max_power));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u16(nvs_handle, NVS_KEY_PID_EXPECT_TEMP, pid_target));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvs_handle, NVS_KEY_BUZZER_VOLUME, buzzer_volume));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvs_handle, NVS_KEY_BUZZER_NOTE, buzzer_note));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvs_handle, NVS_KEY_BRIGHTNESS, brightness));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u16(nvs_handle, NVS_KEY_UPPER_DIVIDER_RESISTANCE, upper_div_r));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_i16(nvs_handle, NVS_KEY_ADC_OFFSET, adc_offset));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvs_handle, NVS_KEY_TILT_THRESHOLD, tilt_threshold));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvs_handle, NVS_KEY_SHUNT_RESISTANCE, shunt_res));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvs_handle, NVS_KEY_MIN_VOLTAGE, min_voltage));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_u8(nvs_handle, NVS_KEY_SOFT_START_TIME, soft_start_time_s));
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_commit(nvs_handle));

        ESP_LOGI("NVS", "NVS variables saved");
        saved = true;
    } else {
        ESP_LOGE("NVS", "Failed to open NVS for saving variables");
    }

    nvs_close(nvs_handle);

    if (!saved) {
        app_state_lock();
        g_state.system.nvs_need_save = true;
        app_state_unlock();
    }
}
