// FreeRTOS shim — maps the fake C API onto esp32sim::Rtos.
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <esp32sim/clock.h>
#include <esp32sim/rtos.h>

#include <string>

namespace {
// Tasks are addressed by name — we encode the registered name in the handle
// using a small static table.
constexpr int kMaxTasks = 16;
struct TaskSlot { bool used = false; std::string name; };
TaskSlot task_slots[kMaxTasks];
int alloc_task_slot(const std::string& name) {
    for (int i = 0; i < kMaxTasks; ++i) {
        if (!task_slots[i].used) {
            task_slots[i].used = true;
            task_slots[i].name = name;
            return i;
        }
    }
    return -1;
}
}  // namespace

extern "C" {

BaseType_t xTaskCreate(
    TaskFunction_t fn,
    const char* name,
    uint32_t /*stack_depth*/,
    void* parameter,
    UBaseType_t /*priority*/,
    TaskHandle_t* created_task) {
    if (!fn || !name) return pdFAIL;
    int slot = alloc_task_slot(name);
    if (slot < 0) return pdFAIL;
    esp32sim::Rtos::instance().register_task(name, fn, parameter);
    if (created_task) {
        // Encode slot in the handle pointer (cast back when needed).
        *created_task = reinterpret_cast<TaskHandle_t>((intptr_t)(slot + 1));
    }
    return pdPASS;
}

BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t fn,
    const char* name,
    uint32_t stack_depth,
    void* parameter,
    UBaseType_t priority,
    TaskHandle_t* created_task,
    BaseType_t /*core_id*/) {
    return xTaskCreate(fn, name, stack_depth, parameter, priority, created_task);
}

void vTaskDelete(TaskHandle_t task) {
    if (!task) return;
    int slot = (int)((intptr_t)task) - 1;
    if (slot < 0 || slot >= kMaxTasks) return;
    task_slots[slot].used = false;
    task_slots[slot].name.clear();
}

void vTaskDelay(TickType_t ticks) {
    // ticks → ms via portTICK_PERIOD_MS=1
    esp32sim::VirtualClock::instance().advance_ms((uint64_t)ticks);
}

QueueHandle_t xQueueCreate(UBaseType_t length, UBaseType_t item_size) {
    int h = esp32sim::Rtos::instance().queue_create((int)item_size, (int)length);
    return reinterpret_cast<QueueHandle_t>((intptr_t)(h + 1));
}

BaseType_t xQueueSend(QueueHandle_t q, const void* item, TickType_t /*ticks*/) {
    int h = (int)((intptr_t)q) - 1;
    return esp32sim::Rtos::instance().queue_send(h, item, 0) ? pdPASS : pdFAIL;
}

BaseType_t xQueueReceive(QueueHandle_t q, void* out, TickType_t /*ticks*/) {
    int h = (int)((intptr_t)q) - 1;
    return esp32sim::Rtos::instance().queue_receive(h, out, 0) ? pdPASS : pdFAIL;
}

UBaseType_t uxQueueMessagesWaiting(QueueHandle_t q) {
    int h = (int)((intptr_t)q) - 1;
    return (UBaseType_t)esp32sim::Rtos::instance().queue_size(h);
}

SemaphoreHandle_t xSemaphoreCreateBinary(void) {
    int h = esp32sim::Rtos::instance().semaphore_create(0, 1);
    return reinterpret_cast<SemaphoreHandle_t>((intptr_t)(h + 1));
}
SemaphoreHandle_t xSemaphoreCreateCounting(UBaseType_t max_count, UBaseType_t initial_count) {
    int h = esp32sim::Rtos::instance().semaphore_create((int)initial_count, (int)max_count);
    return reinterpret_cast<SemaphoreHandle_t>((intptr_t)(h + 1));
}
SemaphoreHandle_t xSemaphoreCreateMutex(void) {
    int h = esp32sim::Rtos::instance().semaphore_create(1, 1);
    return reinterpret_cast<SemaphoreHandle_t>((intptr_t)(h + 1));
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t s, TickType_t /*ticks*/) {
    int h = (int)((intptr_t)s) - 1;
    return esp32sim::Rtos::instance().semaphore_take(h, 0) ? pdPASS : pdFAIL;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t s) {
    int h = (int)((intptr_t)s) - 1;
    return esp32sim::Rtos::instance().semaphore_give(h) ? pdPASS : pdFAIL;
}

}  // extern "C"
