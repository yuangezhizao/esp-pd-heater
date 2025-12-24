#include "app_events.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static QueueHandle_t s_queue = NULL;

void app_events_init(void) {
    if (s_queue == NULL) {
        s_queue = xQueueCreate(16, sizeof(app_event_t));
    }
}

bool app_events_post(const app_event_t *event, TickType_t ticks_to_wait) {
    if (s_queue == NULL || event == NULL) return false;
    return xQueueSend(s_queue, event, ticks_to_wait) == pdTRUE;
}

bool app_events_receive(app_event_t *event, TickType_t ticks_to_wait) {
    if (s_queue == NULL || event == NULL) return false;
    return xQueueReceive(s_queue, event, ticks_to_wait) == pdTRUE;
}

