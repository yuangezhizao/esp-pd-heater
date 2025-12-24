#include "app_buzzer.h"

#include "app_state.h"
#include "bsp/esp-bsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

uint32_t calculate_buzzer_volume(uint8_t percent) {
    return percent * 4095 / 100;
}

void buzzer_start_task(void *pvParameter) {
    app_state_t st = {0};
    app_state_snapshot(&st);

    uint32_t vol = calculate_buzzer_volume(st.config.buzzer_volume);
    bsp_buzzer_set_default(st.config.buzzer_note, vol);
    bsp_buzzer_init();
    vTaskDelay(pdMS_TO_TICKS(250));

    uint8_t note_1 = (st.config.buzzer_note + 6) > 88 ? 82 : st.config.buzzer_note;
    uint8_t note_2 = note_1 + 3;
    uint8_t note_3 = note_1 + 6;

    bsp_buzzer_beep(note_1, vol, 0.2, 0.02, 1);
    bsp_buzzer_beep(note_2, vol, 0.2, 0.02, 1);
    bsp_buzzer_beep(note_3, vol, 0.2, 0.02, 1);
    vTaskDelete(NULL);
}
