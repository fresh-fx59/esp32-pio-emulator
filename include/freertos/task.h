// include/freertos/task.h — fake FreeRTOS task API (T4)
#pragma once

#include <freertos/FreeRTOS.h>

#ifdef __cplusplus
extern "C" {
#endif

BaseType_t xTaskCreate(
    TaskFunction_t fn,
    const char* name,
    uint32_t stack_depth,
    void* parameter,
    UBaseType_t priority,
    TaskHandle_t* created_task);

BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t fn,
    const char* name,
    uint32_t stack_depth,
    void* parameter,
    UBaseType_t priority,
    TaskHandle_t* created_task,
    BaseType_t core_id);

void vTaskDelete(TaskHandle_t task);
void vTaskDelay(TickType_t ticks);

#ifdef __cplusplus
}
#endif
