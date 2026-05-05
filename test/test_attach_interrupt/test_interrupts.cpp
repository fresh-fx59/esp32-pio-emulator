#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

namespace {
int isr_count = 0;
}
void my_isr() { isr_count++; }

void setUp(void) {
    esp32sim::Sim::reset();
    isr_count = 0;
    // Detach all to avoid carry-over from prior tests' listeners.
    for (int i = 0; i < 64; ++i) detachInterrupt((uint8_t)i);
}
void tearDown(void) {}

void test_rising_fires_only_on_low_to_high(void) {
    pinMode(4, INPUT);
    attachInterrupt(digitalPinToInterrupt(4), my_isr, RISING);
    esp32sim::Sim::gpio(4).setLevel(0);  // already 0, no edge
    esp32sim::Sim::gpio(4).setLevel(1);  // 0->1: fires
    esp32sim::Sim::gpio(4).setLevel(0);  // 1->0: no fire (FALLING)
    esp32sim::Sim::gpio(4).setLevel(1);  // 0->1: fires
    TEST_ASSERT_EQUAL_INT(2, isr_count);
}

void test_falling_fires_only_on_high_to_low(void) {
    pinMode(4, INPUT);
    esp32sim::Sim::gpio(4).setLevel(1);
    attachInterrupt(digitalPinToInterrupt(4), my_isr, FALLING);
    esp32sim::Sim::gpio(4).setLevel(0);  // 1->0: fires
    esp32sim::Sim::gpio(4).setLevel(1);  // no
    esp32sim::Sim::gpio(4).setLevel(0);  // fires
    TEST_ASSERT_EQUAL_INT(2, isr_count);
}

void test_change_fires_on_both_edges(void) {
    pinMode(4, INPUT);
    attachInterrupt(digitalPinToInterrupt(4), my_isr, CHANGE);
    esp32sim::Sim::gpio(4).setLevel(1);
    esp32sim::Sim::gpio(4).setLevel(0);
    esp32sim::Sim::gpio(4).setLevel(1);
    TEST_ASSERT_EQUAL_INT(3, isr_count);
}

void test_detachInterrupt_silences_isr(void) {
    pinMode(4, INPUT);
    attachInterrupt(digitalPinToInterrupt(4), my_isr, RISING);
    esp32sim::Sim::gpio(4).setLevel(1);
    detachInterrupt(digitalPinToInterrupt(4));
    esp32sim::Sim::gpio(4).setLevel(0);
    esp32sim::Sim::gpio(4).setLevel(1);
    TEST_ASSERT_EQUAL_INT(1, isr_count);  // only the pre-detach one
}

void test_no_isr_for_non_matching_mode(void) {
    pinMode(4, INPUT);
    attachInterrupt(digitalPinToInterrupt(4), my_isr, FALLING);
    esp32sim::Sim::gpio(4).setLevel(1);  // RISING — not FALLING
    TEST_ASSERT_EQUAL_INT(0, isr_count);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_rising_fires_only_on_low_to_high);
    RUN_TEST(test_falling_fires_only_on_high_to_low);
    RUN_TEST(test_change_fires_on_both_edges);
    RUN_TEST(test_detachInterrupt_silences_isr);
    RUN_TEST(test_no_isr_for_non_matching_mode);
    return UNITY_END();
}
