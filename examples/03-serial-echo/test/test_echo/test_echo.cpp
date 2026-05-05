#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) { esp32sim::Sim::reset(); }
void tearDown(void) {}

void test_echo_uppercase(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::uart(0).inject("hello");
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_STRING("HELLO", esp32sim::Sim::uart(0).drainTx().c_str());
}

void test_passthrough_non_lowercase(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::uart(0).inject("Hi 42!");
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_STRING("HI 42!", esp32sim::Sim::uart(0).drainTx().c_str());
}

void test_no_input_no_output(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_STRING("", esp32sim::Sim::uart(0).drainTx().c_str());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_echo_uppercase);
    RUN_TEST(test_passthrough_non_lowercase);
    RUN_TEST(test_no_input_no_output);
    return UNITY_END();
}
