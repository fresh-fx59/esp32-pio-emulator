#include <Arduino.h>
#include <esp32_hwtimer.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

namespace {
int isr_count = 0;
}
void timer_isr() { isr_count++; }

void setUp(void) {
    esp32sim::Sim::reset();
    isr_count = 0;
}
void tearDown(void) {}

void test_timer_oneshot_fires_at_alarm(void) {
    // 1us tick (divider=80 on 80MHz). Alarm at 1000 ticks = 1ms.
    auto* t = timerBegin(0, 80, true);
    timerAttachInterrupt(t, timer_isr, true);
    timerAlarmWrite(t, 1000, /*autoreload=*/false);
    timerAlarmEnable(t);
    esp32sim::Sim::advanceUs(500);
    TEST_ASSERT_EQUAL_INT(0, isr_count);
    esp32sim::Sim::advanceUs(500);
    TEST_ASSERT_EQUAL_INT(1, isr_count);
    // No autoreload — shouldn't fire again.
    esp32sim::Sim::advanceUs(2000);
    TEST_ASSERT_EQUAL_INT(1, isr_count);
}

void test_timer_periodic_autoreload(void) {
    auto* t = timerBegin(0, 80, true);
    timerAttachInterrupt(t, timer_isr, true);
    timerAlarmWrite(t, 500, /*autoreload=*/true);  // every 500us
    timerAlarmEnable(t);
    esp32sim::Sim::advanceUs(500);
    TEST_ASSERT_EQUAL_INT(1, isr_count);
    esp32sim::Sim::advanceUs(500);
    TEST_ASSERT_EQUAL_INT(2, isr_count);
    esp32sim::Sim::advanceUs(1500);
    TEST_ASSERT_EQUAL_INT(5, isr_count);
}

void test_timer_disable_stops_firing(void) {
    auto* t = timerBegin(0, 80, true);
    timerAttachInterrupt(t, timer_isr, true);
    timerAlarmWrite(t, 100, true);
    timerAlarmEnable(t);
    esp32sim::Sim::advanceUs(250);
    TEST_ASSERT_EQUAL_INT(2, isr_count);
    timerAlarmDisable(t);
    esp32sim::Sim::advanceUs(1000);
    TEST_ASSERT_EQUAL_INT(2, isr_count);  // no further firings
}

void test_two_timers_independent(void) {
    int a = 0, b = 0;
    auto* ta = timerBegin(0, 80, true);
    auto* tb = timerBegin(1, 80, true);
    timerAttachInterrupt(ta, +[](){ /* placeholder */ }, true);
    timerAttachInterrupt(tb, +[](){ /* placeholder */ }, true);
    // Use captureless lambdas via function-pointer closures isn't trivial;
    // instead use file-scope counters.
    static int counter_a, counter_b;
    counter_a = counter_b = 0;
    timerAttachInterrupt(ta, +[](){ counter_a++; }, true);
    timerAttachInterrupt(tb, +[](){ counter_b++; }, true);
    timerAlarmWrite(ta, 100, true);
    timerAlarmWrite(tb, 250, true);
    timerAlarmEnable(ta);
    timerAlarmEnable(tb);
    esp32sim::Sim::advanceUs(500);
    TEST_ASSERT_EQUAL_INT(5, counter_a);
    TEST_ASSERT_EQUAL_INT(2, counter_b);
    (void)a; (void)b;
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_timer_oneshot_fires_at_alarm);
    RUN_TEST(test_timer_periodic_autoreload);
    RUN_TEST(test_timer_disable_stops_firing);
    RUN_TEST(test_two_timers_independent);
    return UNITY_END();
}
