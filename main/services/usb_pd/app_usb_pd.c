#include "app_usb_pd.h"

#include <math.h>
#include <string.h>

#include "Adafruit_HUSB238.h"
#include "app_strbuf.h"
#include "app_state.h"
#include "ch32x035_pd.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define TAG "usb_pd"

typedef struct {
    pd_chip_type_t chip_type;  // 芯片类型
    bool is_attached;          // 是否连接
    bool is_cc2;               // 是否CC2连接
    pd_voltage_t max_voltage;  // 最大支持电压
    char info_text[512];       // 状态信息文本
    SemaphoreHandle_t info_text_mutex;
    void *chip_handle;  // 芯片句柄(husb238或ch32x035)
} pd_state_t;

static pd_state_t pd_state = {0};

// 电压值映射表
static const uint8_t voltage_map[] = {5, 9, 12, 15, 18, 20};

void app_pd_init(i2c_bus_handle_t i2c_bus) {
    if (pd_state.info_text_mutex == NULL) {
        pd_state.info_text_mutex = xSemaphoreCreateMutex();
    }

    if (i2c_bus == NULL) {
        ESP_LOGE(TAG, "app_pd_init: i2c_bus is NULL");
        pd_state.chip_type = PD_CHIP_HUSB238;
        pd_state.chip_handle = NULL;
        return;
    }

    // Prefer probing address first (no "unexpected nack" logs on missing device)
    bool has_ch32x035 = false;
    i2c_master_bus_handle_t internal_bus = i2c_bus_get_internal_bus_handle(i2c_bus);
    if (internal_bus != NULL) {
        esp_err_t probe_ret = i2c_master_probe(internal_bus, CH32X035_PD_ADDR, 50);
        has_ch32x035 = (probe_ret == ESP_OK);
    }

    if (has_ch32x035) {
        pd_state.chip_handle = ch32x035_pd_create(i2c_bus, CH32X035_PD_ADDR);
        uint8_t device_id = 0;
        esp_err_t ret = ch32x035_pd_get_device_id(pd_state.chip_handle, &device_id);
        if (ret == ESP_OK && device_id == 0x35) {
            ESP_LOGI(TAG, "CH32X035 verified with device ID: 0x%02x", device_id);
            pd_state.chip_type = PD_CHIP_CH32X035;
            return;
        }

        ESP_LOGW(TAG, "CH32X035 probe ok but device ID invalid (id=0x%02x, err=%s); using HUSB238", device_id, esp_err_to_name(ret));
        ch32x035_pd_handle_t temp_handle = pd_state.chip_handle;
        ch32x035_pd_delete(&temp_handle);
    }

    pd_state.chip_type = PD_CHIP_HUSB238;
    pd_state.chip_handle = husb238_create(i2c_bus, HUSB238_I2CADDR_DEFAULT);
}

void app_pd_update(void) {
    char tmp[sizeof(pd_state.info_text)];
    app_strbuf_t sb;
    app_strbuf_init(&sb, tmp, sizeof(tmp));

    app_state_t st = {0};
    app_state_snapshot(&st);

    // 临时加个 DEBUG 调试信息
    app_strbuf_appendf(&sb, "CHIP TEMP: %.2f°C\nHEAT TEMP: %.2f°C\nTILT ANGLE: %.2f°\n",
                       st.temp.chip,
                       st.temp.pt1000,
                       st.lis2dh12.tilt_angle);

    if (pd_state.chip_type == PD_CHIP_HUSB238) {
        husb238_handle_t husb238 = (husb238_handle_t)pd_state.chip_handle;

        // PD 芯片类型
        app_strbuf_append(&sb, "PD CHIP: HUSB238\n\n");

        // 更新连接状态
        pd_state.is_attached = husb238_is_attached(husb238);
        app_strbuf_append(&sb, "-------- STATUS: --------\n");
        if (pd_state.is_attached) {
            pd_state.is_cc2 = husb238_get_cc_direction(husb238);
            app_strbuf_append(&sb, pd_state.is_cc2 ? "CC2 Attached\n" : "CC1 Attached\n");
        } else {
            app_strbuf_append(&sb, "Detached\n");
        }

        if (pd_state.is_attached) {
            // 添加 PD SRC 信息
            app_strbuf_append(&sb, "-------- PD SRC: --------\n");
            HUSB238_VoltageSetting src_voltage;
            HUSB238_CurrentSetting src_current;
            husb238_get_pd_src_voltage(husb238, &src_voltage);
            husb238_get_pd_src_current(husb238, &src_current);
            app_strbuf_append(&sb, get_voltage_setting_string(src_voltage));
            app_strbuf_append(&sb, " - ");
            app_strbuf_append(&sb, get_current_setting_string(src_current));
            app_strbuf_append(&sb, "\n");

            // 检测支持的电压
            app_strbuf_append(&sb, "--------- PDO: ----------\n");
            HUSB238_PDSelection pd_selection_list[] = {PD_SRC_5V, PD_SRC_9V, PD_SRC_12V, PD_SRC_15V, PD_SRC_18V, PD_SRC_20V};

            pd_state.max_voltage = PD_VOLT_5V;
            for (size_t i = 0; i < sizeof(pd_selection_list) / sizeof(pd_selection_list[0]); i++) {
                bool is_voltage_detected = husb238_is_voltage_detected(husb238, pd_selection_list[i]);
                app_strbuf_append(&sb, get_pd_selection_string(pd_selection_list[i]));
                app_strbuf_append(&sb, is_voltage_detected ? " - Y - " : " - N");

                if (is_voltage_detected) {
                    pd_state.max_voltage = i;
                    HUSB238_CurrentSetting currentDetected;
                    husb238_current_detected(husb238, pd_selection_list[i], &currentDetected);
                    app_strbuf_append(&sb, get_current_setting_string(currentDetected));
                }
                app_strbuf_append(&sb, "\n");
            }

            // 添加 5V 信息
            app_strbuf_append(&sb, "------ 5V Contract: ------\n");
            bool is_5v_voltage_detected = husb238_get_5v_contract_voltage(husb238);
            app_strbuf_append(&sb, is_5v_voltage_detected ? "5V - " : "other voltage - ");
            HUSB238_5VCurrentContract _5v_contract_current;
            husb238_get_5v_contract_current(husb238, &_5v_contract_current);
            app_strbuf_append(&sb, get_5v_contract_current_string(_5v_contract_current));
            app_strbuf_append(&sb, "\n");

            // 添加 PD 响应信息
            app_strbuf_append(&sb, "-------- PD RES: --------\n");
            HUSB238_ResponseCodes pd_response;
            husb238_get_pd_response(husb238, &pd_response);
            app_strbuf_append(&sb, get_pd_response_string(pd_response));
        }
    } else {
        // ch32x035_pd_handle_t ch32x035 = (ch32x035_pd_handle_t)pd_state.chip_handle;
        app_strbuf_append(&sb, "PD CHIP: CH32X035");

        // // 获取设备 ID
        // uint8_t device_id = 0;
        // if (ch32x035_pd_get_device_id(ch32x035, &device_id) == ESP_OK) {
        //     char id_str[32];
        //     snprintf(id_str, sizeof(id_str), "CH32X035 ID: 0x%02X", device_id);
        //     strcat(pd_state.info_text, id_str);
        // }
    }

    if (pd_state.info_text_mutex != NULL && xSemaphoreTake(pd_state.info_text_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        strncpy(pd_state.info_text, tmp, sizeof(pd_state.info_text) - 1);
        pd_state.info_text[sizeof(pd_state.info_text) - 1] = '\0';
        xSemaphoreGive(pd_state.info_text_mutex);
    } else {
        // Best-effort update (still keep it NUL-terminated)
        strncpy(pd_state.info_text, tmp, sizeof(pd_state.info_text) - 1);
        pd_state.info_text[sizeof(pd_state.info_text) - 1] = '\0';
    }
}

bool app_pd_request_voltage(pd_voltage_t voltage) {
    if (voltage >= PD_VOLT_MAX) return false;

    if (pd_state.chip_type == PD_CHIP_HUSB238) {
        husb238_handle_t husb238 = (husb238_handle_t)pd_state.chip_handle;
        HUSB238_PDSelection pd_selections[] = {PD_SRC_5V, PD_SRC_9V, PD_SRC_12V, PD_SRC_15V, PD_SRC_18V, PD_SRC_20V};
        husb238_select_pd(husb238, pd_selections[voltage]);
        husb238_request_pd(husb238);
        return true;
    } else {
        ch32x035_pd_handle_t ch32x035 = (ch32x035_pd_handle_t)pd_state.chip_handle;
        ch32x035_pd_request_voltage(ch32x035, voltage_map[voltage]);
        return true;
    }
}

void app_pd_request_max_voltage(void) {
    if (pd_state.chip_type == PD_CHIP_HUSB238) {
        husb238_handle_t husb238 = (husb238_handle_t)pd_state.chip_handle;
        HUSB238_PDSelection pd_selections[] = {PD_SRC_5V, PD_SRC_9V, PD_SRC_12V, PD_SRC_15V, PD_SRC_18V, PD_SRC_20V};

        husb238_select_pd(husb238, pd_selections[pd_state.max_voltage]);
        husb238_request_pd(husb238);
    } else {
        ch32x035_pd_handle_t ch32x035 = (ch32x035_pd_handle_t)pd_state.chip_handle;
        // 电压映射表
        const uint8_t voltage_steps[] = {9, 12, 15, 20};

        // 遍历每个电压档位
        for (int i = 0; i < sizeof(voltage_steps) / sizeof(voltage_steps[0]); i++) {
            float target_voltage = voltage_steps[i];
            // 如果当前测量电压已经在目标电压附近或更高，跳过此档位
            app_state_t st = {0};
            app_state_snapshot(&st);
            if (st.power.voltage >= (target_voltage - 2.0f)) {
                ESP_LOGI(TAG, "Skip %dV (current: %.1fV)", voltage_steps[i], st.power.voltage);
                continue;
            }

            ESP_LOGI(TAG, "Request %dV (current: %.1fV)", voltage_steps[i], st.power.voltage);
            ch32x035_pd_request_voltage(ch32x035, voltage_steps[i]);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

bool app_pd_get_info_text_copy(char *out, size_t out_len) {
    if (out == NULL || out_len == 0) return false;

    out[0] = '\0';
    if (pd_state.info_text_mutex != NULL && xSemaphoreTake(pd_state.info_text_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        strncpy(out, pd_state.info_text, out_len - 1);
        out[out_len - 1] = '\0';
        xSemaphoreGive(pd_state.info_text_mutex);
        return true;
    }

    // Best-effort copy (still keep it NUL-terminated)
    strncpy(out, pd_state.info_text, out_len - 1);
    out[out_len - 1] = '\0';
    return true;
}
