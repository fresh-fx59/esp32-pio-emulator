#include <esp32sim/clock.h>
#include <esp32sim/event_log.h>
#include <esp32sim/gpio.h>
#include <unity.h>

using namespace esp32sim;

void setUp(void) {
    VirtualClock::instance().reset();
    EventLog::instance().reset();
    PinRegistry::instance().reset();
}
void tearDown(void) {}

void test_pin_starts_low_input(void) {
    auto& r = PinRegistry::instance();
    TEST_ASSERT_EQUAL_INT(0, r.get_level(2));
    TEST_ASSERT_EQUAL_INT((int)PinMode::INPUT, (int)r.get_mode(2));
}

void test_set_level_records_in_event_log(void) {
    auto& r = PinRegistry::instance();
    r.set_mode(2, PinMode::OUTPUT);
    r.set_level(2, 1);
    TEST_ASSERT_EQUAL_INT(1, r.get_level(2));
    auto writes = EventLog::instance().by_kind(EventKind::GPIO_WRITE);
    TEST_ASSERT_EQUAL_size_t(1, writes.size());
    TEST_ASSERT_EQUAL_INT(2, writes[0].pin);
    TEST_ASSERT_EQUAL_INT(1, writes[0].value);
}

void test_set_mode_records_in_event_log(void) {
    PinRegistry::instance().set_mode(4, PinMode::INPUT_PULLUP);
    auto modes = EventLog::instance().by_kind(EventKind::GPIO_PIN_MODE);
    TEST_ASSERT_EQUAL_size_t(1, modes.size());
    TEST_ASSERT_EQUAL_INT(4, modes[0].pin);
    TEST_ASSERT_EQUAL_INT((int)PinMode::INPUT_PULLUP, modes[0].value);
}

void test_pullup_pin_reads_high_before_any_drive(void) {
    auto& r = PinRegistry::instance();
    r.set_mode(5, PinMode::INPUT_PULLUP);
    TEST_ASSERT_EQUAL_INT(1, r.get_level(5));
}

void test_pulldown_pin_reads_low(void) {
    auto& r = PinRegistry::instance();
    r.set_mode(5, PinMode::INPUT_PULLDOWN);
    TEST_ASSERT_EQUAL_INT(0, r.get_level(5));
}

void test_listener_fires_on_level_change(void) {
    auto& r = PinRegistry::instance();
    int rising = 0, falling = 0;
    r.add_listener(2, [&](int old_lvl, int new_lvl) {
        if (old_lvl == 0 && new_lvl == 1) rising++;
        if (old_lvl == 1 && new_lvl == 0) falling++;
    });
    r.set_level(2, 1);
    r.set_level(2, 0);
    r.set_level(2, 0);  // no change — listener not called
    TEST_ASSERT_EQUAL_INT(1, rising);
    TEST_ASSERT_EQUAL_INT(1, falling);
}

void test_invalid_pin_silent_noop(void) {
    auto& r = PinRegistry::instance();
    TEST_ASSERT_EQUAL_INT(0, r.get_level(100));
    r.set_level(100, 1);
    TEST_ASSERT_EQUAL_INT(0, r.get_level(100));
}

void test_reset_clears_state_and_listeners(void) {
    auto& r = PinRegistry::instance();
    bool fired = false;
    r.set_level(2, 1);
    r.add_listener(3, [&fired](int, int) { fired = true; });
    r.reset();
    TEST_ASSERT_EQUAL_INT(0, r.get_level(2));
    r.set_level(3, 1);
    TEST_ASSERT_FALSE(fired);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_pin_starts_low_input);
    RUN_TEST(test_set_level_records_in_event_log);
    RUN_TEST(test_set_mode_records_in_event_log);
    RUN_TEST(test_pullup_pin_reads_high_before_any_drive);
    RUN_TEST(test_pulldown_pin_reads_low);
    RUN_TEST(test_listener_fires_on_level_change);
    RUN_TEST(test_invalid_pin_silent_noop);
    RUN_TEST(test_reset_clears_state_and_listeners);
    return UNITY_END();
}
