#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) { esp32sim::Sim::reset(); }
void tearDown(void) {}

void test_setup_initializes_channel(void) {
    esp32sim::Sim::runSetup();
    TEST_ASSERT_TRUE(esp32sim::Pwm::instance().in_use(0));
    TEST_ASSERT_EQUAL_UINT32(1000, esp32sim::Pwm::instance().frequency(0));
    TEST_ASSERT_EQUAL_UINT32(0, esp32sim::Pwm::instance().duty(0));
}

void test_duty_increases_across_iterations(void) {
    esp32sim::Sim::runSetup();
    // Each iteration advances 10ms and increments duty by 1.
    for (int i = 0; i < 50; ++i) {
        esp32sim::Sim::advanceMs(10);
        esp32sim::Sim::runLoop();
    }
    TEST_ASSERT_EQUAL_UINT32(50, esp32sim::Pwm::instance().duty(0));
}

void test_duty_reverses_at_max(void) {
    esp32sim::Sim::runSetup();
    // Run 300 iterations: 0..255 (256 ms) then 255..down for 44 more = duty 211.
    for (int i = 0; i < 300; ++i) {
        esp32sim::Sim::advanceMs(10);
        esp32sim::Sim::runLoop();
    }
    uint32_t d = esp32sim::Pwm::instance().duty(0);
    TEST_ASSERT_TRUE(d < 255);
    TEST_ASSERT_TRUE(d > 0);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_setup_initializes_channel);
    RUN_TEST(test_duty_increases_across_iterations);
    RUN_TEST(test_duty_reverses_at_max);
    return UNITY_END();
}
