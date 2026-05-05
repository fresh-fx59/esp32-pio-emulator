#include <esp_sleep.h>
#include <esp32sim/sleep.h>
#include <unity.h>

void setUp(void) { esp32sim::Sleep::instance().reset(); }
void tearDown(void) {}

void test_default_wake_cause_undefined(void) {
    TEST_ASSERT_EQUAL_INT(ESP_SLEEP_WAKEUP_UNDEFINED, (int)esp_sleep_get_wakeup_cause());
}

void test_deep_sleep_records_duration(void) {
    esp_sleep_enable_timer_wakeup(5000000);  // 5 seconds in us
    esp_deep_sleep_start();
    TEST_ASSERT_EQUAL_UINT64(5000000, esp32sim::Sleep::instance().last_sleep_us());
    TEST_ASSERT_EQUAL_INT(1, esp32sim::Sleep::instance().deep_sleep_count());
}

void test_wake_cause_set_by_test(void) {
    esp32sim::Sleep::instance().set_next_wake_cause(esp32sim::WakeCause::TIMER);
    esp_sleep_enable_timer_wakeup(1000000);
    esp_deep_sleep_start();
    TEST_ASSERT_EQUAL_INT(ESP_SLEEP_WAKEUP_TIMER, (int)esp_sleep_get_wakeup_cause());
}

void test_multiple_deep_sleep_cycles(void) {
    esp_sleep_enable_timer_wakeup(1000);
    esp_deep_sleep_start();
    esp_sleep_enable_timer_wakeup(2000);
    esp_deep_sleep_start();
    esp_sleep_enable_timer_wakeup(3000);
    esp_deep_sleep_start();
    TEST_ASSERT_EQUAL_INT(3, esp32sim::Sleep::instance().deep_sleep_count());
    TEST_ASSERT_EQUAL_UINT64(3000, esp32sim::Sleep::instance().last_sleep_us());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_default_wake_cause_undefined);
    RUN_TEST(test_deep_sleep_records_duration);
    RUN_TEST(test_wake_cause_set_by_test);
    RUN_TEST(test_multiple_deep_sleep_cycles);
    return UNITY_END();
}
