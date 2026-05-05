// include/freertos/queue.h — fake FreeRTOS queue API (T4)
#pragma once

#include <freertos/FreeRTOS.h>

#ifdef __cplusplus
extern "C" {
#endif

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size);
BaseType_t    xQueueSend(QueueHandle_t q, const void* item, TickType_t ticks);
BaseType_t    xQueueReceive(QueueHandle_t q, void* out, TickType_t ticks);
UBaseType_t   uxQueueMessagesWaiting(QueueHandle_t q);

#ifdef __cplusplus
}
#endif
