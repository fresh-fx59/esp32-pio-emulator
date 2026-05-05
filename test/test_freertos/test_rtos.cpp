#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp32sim/rtos.h>
#include <unity.h>

void setUp(void) { esp32sim::Rtos::instance().reset(); }
void tearDown(void) {}

// ---- Tasks ----

void test_xTaskCreate_registers_task(void) {
    TaskHandle_t h = nullptr;
    auto rc = xTaskCreate(
        [](void*) {}, "worker", 4096, nullptr, 5, &h);
    TEST_ASSERT_EQUAL(pdPASS, rc);
    TEST_ASSERT_NOT_NULL(h);
    TEST_ASSERT_TRUE(esp32sim::Rtos::instance().task_exists("worker"));
}

void test_run_task_iteration_invokes_fn(void) {
    static int count;
    count = 0;
    TaskHandle_t h = nullptr;
    xTaskCreate([](void*) { count++; }, "ticker", 4096, nullptr, 5, &h);
    esp32sim::Rtos::instance().run_task_iteration("ticker");
    esp32sim::Rtos::instance().run_task_iteration("ticker");
    TEST_ASSERT_EQUAL_INT(2, count);
}

// ---- Queues ----

void test_xQueueSend_xQueueReceive(void) {
    auto q = xQueueCreate(4, sizeof(int));
    int v = 42;
    TEST_ASSERT_EQUAL(pdPASS, xQueueSend(q, &v, 0));
    int out = 0;
    TEST_ASSERT_EQUAL(pdPASS, xQueueReceive(q, &out, 0));
    TEST_ASSERT_EQUAL_INT(42, out);
}

void test_xQueueReceive_empty_returns_pdFAIL(void) {
    auto q = xQueueCreate(4, sizeof(int));
    int out = 0;
    TEST_ASSERT_EQUAL(pdFAIL, xQueueReceive(q, &out, 0));
}

void test_xQueueSend_full_returns_pdFAIL(void) {
    auto q = xQueueCreate(2, sizeof(int));
    int a = 1, b = 2, c = 3;
    xQueueSend(q, &a, 0);
    xQueueSend(q, &b, 0);
    TEST_ASSERT_EQUAL(pdFAIL, xQueueSend(q, &c, 0));
}

// ---- Semaphores ----

void test_binary_semaphore_starts_empty(void) {
    auto s = xSemaphoreCreateBinary();
    TEST_ASSERT_EQUAL(pdFAIL, xSemaphoreTake(s, 0));
    xSemaphoreGive(s);
    TEST_ASSERT_EQUAL(pdPASS, xSemaphoreTake(s, 0));
}

void test_counting_semaphore(void) {
    auto s = xSemaphoreCreateCounting(3, 2);
    TEST_ASSERT_EQUAL(pdPASS, xSemaphoreTake(s, 0));
    TEST_ASSERT_EQUAL(pdPASS, xSemaphoreTake(s, 0));
    TEST_ASSERT_EQUAL(pdFAIL, xSemaphoreTake(s, 0));
    xSemaphoreGive(s);
    TEST_ASSERT_EQUAL(pdPASS, xSemaphoreTake(s, 0));
}

void test_mutex_starts_available(void) {
    auto m = xSemaphoreCreateMutex();
    TEST_ASSERT_EQUAL(pdPASS, xSemaphoreTake(m, 0));
    xSemaphoreGive(m);
}

// ---- vTaskDelay advances virtual clock ----

void test_vTaskDelay_advances_clock(void) {
    uint32_t before = millis();
    vTaskDelay(pdMS_TO_TICKS(100));
    uint32_t after = millis();
    TEST_ASSERT_EQUAL_UINT32(100, after - before);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_xTaskCreate_registers_task);
    RUN_TEST(test_run_task_iteration_invokes_fn);
    RUN_TEST(test_xQueueSend_xQueueReceive);
    RUN_TEST(test_xQueueReceive_empty_returns_pdFAIL);
    RUN_TEST(test_xQueueSend_full_returns_pdFAIL);
    RUN_TEST(test_binary_semaphore_starts_empty);
    RUN_TEST(test_counting_semaphore);
    RUN_TEST(test_mutex_starts_available);
    RUN_TEST(test_vTaskDelay_advances_clock);
    return UNITY_END();
}
