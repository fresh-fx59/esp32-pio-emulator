// include/freertos/FreeRTOS.h — fake FreeRTOS (T4 cooperative shim)
#pragma once

#include <stdint.h>

#define portTICK_PERIOD_MS 1
#define pdMS_TO_TICKS(ms)  (ms)
#define pdTRUE  1
#define pdFALSE 0
#define pdPASS  1
#define pdFAIL  0

typedef long          BaseType_t;
typedef unsigned long UBaseType_t;
typedef uint32_t      TickType_t;

typedef void* TaskHandle_t;
typedef void* QueueHandle_t;
typedef void* SemaphoreHandle_t;

typedef void (*TaskFunction_t)(void* parameter);
