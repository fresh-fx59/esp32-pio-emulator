#include <Arduino.h>
#include <esp32sim/clock.h>
#include <esp32sim/event_log.h>
#include <esp32sim/gpio.h>
#include <unity.h>

void setUp(void) {
    esp32sim::VirtualClock::instance().reset();
    esp32sim::EventLog::instance().reset();
    esp32sim::PinRegistry::instance().reset();
}
void tearDown(void) {}

void test_macros_defined(void) {
    TEST_ASSERT_EQUAL_INT(0, LOW);
    TEST_ASSERT_EQUAL_INT(1, HIGH);
}

void test_digitalWrite_then_digitalRead(void) {
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    TEST_ASSERT_EQUAL_INT(HIGH, digitalRead(2));
    digitalWrite(2, LOW);
    TEST_ASSERT_EQUAL_INT(LOW, digitalRead(2));
}

void test_pinMode_records_in_event_log(void) {
    pinMode(3, INPUT_PULLUP);
    auto modes = esp32sim::EventLog::instance().by_kind(esp32sim::EventKind::GPIO_PIN_MODE);
    TEST_ASSERT_EQUAL_size_t(1, modes.size());
    TEST_ASSERT_EQUAL_INT(3, modes[0].pin);
    TEST_ASSERT_EQUAL_INT((int)esp32sim::PinMode::INPUT_PULLUP, modes[0].value);
}

void test_input_pullup_reads_high_by_default(void) {
    pinMode(4, INPUT_PULLUP);
    TEST_ASSERT_EQUAL_INT(HIGH, digitalRead(4));
}

void test_millis_micros_advance_with_delay(void) {
    TEST_ASSERT_EQUAL_UINT32(0, millis());
    delay(50);
    TEST_ASSERT_EQUAL_UINT32(50, millis());
    TEST_ASSERT_EQUAL_UINT32(50000, micros());
    delayMicroseconds(123);
    TEST_ASSERT_EQUAL_UINT32(50123, micros());
}

void test_yield_is_noop(void) {
    yield();  // must compile & link, observable: nothing changes
    TEST_ASSERT_EQUAL_UINT32(0, millis());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_macros_defined);
    RUN_TEST(test_digitalWrite_then_digitalRead);
    RUN_TEST(test_pinMode_records_in_event_log);
    RUN_TEST(test_input_pullup_reads_high_by_default);
    RUN_TEST(test_millis_micros_advance_with_delay);
    RUN_TEST(test_yield_is_noop);
    return UNITY_END();
}
