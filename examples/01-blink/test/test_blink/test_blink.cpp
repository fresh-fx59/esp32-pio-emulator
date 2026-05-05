#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) { esp32sim::Sim::reset(); }
void tearDown(void) {}

void test_led_starts_off(void) {
    esp32sim::Sim::runSetup();
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());
}

void test_led_toggles_at_1hz(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());

    esp32sim::Sim::advanceMs(500);
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(2).level());

    esp32sim::Sim::advanceMs(500);
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());

    esp32sim::Sim::advanceMs(500);
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(2).level());
}

void test_n_toggles_in_n_seconds(void) {
    esp32sim::Sim::runSetup();
    int toggles = 0;
    int last = LOW;
    for (int i = 0; i < 20; ++i) {
        esp32sim::Sim::advanceMs(500);
        esp32sim::Sim::runLoop();
        int now = esp32sim::Sim::gpio(2).level();
        if (now != last) toggles++;
        last = now;
    }
    // 10 seconds of virtual time at 1Hz toggle rate → 20 toggles
    TEST_ASSERT_INT_WITHIN(1, 20, toggles);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_led_starts_off);
    RUN_TEST(test_led_toggles_at_1hz);
    RUN_TEST(test_n_toggles_in_n_seconds);
    return UNITY_END();
}
