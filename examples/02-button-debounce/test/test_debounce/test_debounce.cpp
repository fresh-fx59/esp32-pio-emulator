#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) { esp32sim::Sim::reset(); }
void tearDown(void) {}

static void run_loops_for(uint64_t ms) {
    for (uint64_t t = 0; t < ms; ++t) {
        esp32sim::Sim::runLoop();
        esp32sim::Sim::advanceMs(1);
    }
}

void test_clean_press_produces_one_edge(void) {
    esp32sim::Sim::runSetup();
    // INPUT_PULLUP — released = HIGH. Press = LOW.
    esp32sim::Sim::gpio(4).setLevel(LOW);
    run_loops_for(100);  // > 50ms debounce window
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());
}

void test_release_returns_led_to_high(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::gpio(4).setLevel(LOW);
    run_loops_for(100);
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());
    esp32sim::Sim::gpio(4).setLevel(HIGH);
    run_loops_for(100);
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(2).level());
}

void test_bounce_within_window_does_not_trigger(void) {
    esp32sim::Sim::runSetup();
    auto pin4 = esp32sim::Sim::gpio(4);
    // Bounce LOW/HIGH for ~10ms (well under 50ms debounce window) then
    // settle back to HIGH. LED should stay HIGH.
    for (int i = 0; i < 5; ++i) {
        pin4.setLevel(LOW);
        esp32sim::Sim::runLoop();
        esp32sim::Sim::advanceMs(1);
        pin4.setLevel(HIGH);
        esp32sim::Sim::runLoop();
        esp32sim::Sim::advanceMs(1);
    }
    // Now sit at HIGH for 100ms total (already 10ms in)
    run_loops_for(100);
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(2).level());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_clean_press_produces_one_edge);
    RUN_TEST(test_release_returns_led_to_high);
    RUN_TEST(test_bounce_within_window_does_not_trigger);
    return UNITY_END();
}
