#include <Arduino.h>
#include <HardwareSerial.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

namespace {
int setup_count = 0;
int loop_count = 0;
}  // namespace

extern "C" void setup(void) {
    setup_count++;
    pinMode(2, OUTPUT);
}

extern "C" void loop(void) {
    loop_count++;
    digitalWrite(2, (loop_count % 2 == 1) ? HIGH : LOW);
    delay(100);
}

void setUp(void) {
    esp32sim::Sim::reset();
    setup_count = 0;
    loop_count = 0;
}
void tearDown(void) {}

void test_runSetup_invokes_user_setup(void) {
    esp32sim::Sim::runSetup();
    TEST_ASSERT_EQUAL_INT(1, setup_count);
}

void test_runLoop_default_one_iteration(void) {
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(1, loop_count);
}

void test_runLoop_n_iterations(void) {
    esp32sim::Sim::runLoop(5);
    TEST_ASSERT_EQUAL_INT(5, loop_count);
}

void test_gpio_level(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(2).level());
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());
}

void test_advanceMs(void) {
    esp32sim::Sim::advanceMs(500);
    TEST_ASSERT_EQUAL_UINT64(500, esp32sim::Sim::nowMs());
}

void test_runUntil_predicate_true(void) {
    esp32sim::Sim::runSetup();
    bool found = esp32sim::Sim::runUntil(
        []() { return esp32sim::Sim::gpio(2).level() == HIGH; },
        /*timeoutMs*/ 1000);
    TEST_ASSERT_TRUE(found);
}

void test_runUntil_timeout(void) {
    esp32sim::Sim::runSetup();
    bool found = esp32sim::Sim::runUntil(
        []() { return esp32sim::Sim::gpio(99).level() == HIGH; },  // never true
        /*timeoutMs*/ 50);
    TEST_ASSERT_FALSE(found);
}

void test_uart_drain_tx(void) {
    Serial.print("hi");
    TEST_ASSERT_EQUAL_STRING("hi", esp32sim::Sim::uart(0).drainTx().c_str());
}

void test_uart_inject(void) {
    esp32sim::Sim::uart(0).inject("Q");
    TEST_ASSERT_EQUAL_INT('Q', Serial.read());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_runSetup_invokes_user_setup);
    RUN_TEST(test_runLoop_default_one_iteration);
    RUN_TEST(test_runLoop_n_iterations);
    RUN_TEST(test_gpio_level);
    RUN_TEST(test_advanceMs);
    RUN_TEST(test_runUntil_predicate_true);
    RUN_TEST(test_runUntil_timeout);
    RUN_TEST(test_uart_drain_tx);
    RUN_TEST(test_uart_inject);
    return UNITY_END();
}
