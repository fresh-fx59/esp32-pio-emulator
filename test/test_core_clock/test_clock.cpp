#include <esp32sim/clock.h>
#include <unity.h>

using esp32sim::VirtualClock;

void setUp(void) { VirtualClock::instance().reset(); }
void tearDown(void) {}

void test_clock_starts_at_zero(void) {
    TEST_ASSERT_EQUAL_UINT64(0, VirtualClock::instance().now_us());
}

void test_advance_increments_now_us(void) {
    VirtualClock::instance().advance_us(123);
    TEST_ASSERT_EQUAL_UINT64(123, VirtualClock::instance().now_us());
    VirtualClock::instance().advance_us(877);
    TEST_ASSERT_EQUAL_UINT64(1000, VirtualClock::instance().now_us());
}

void test_advance_ms_converts_correctly(void) {
    VirtualClock::instance().advance_ms(5);
    TEST_ASSERT_EQUAL_UINT64(5000, VirtualClock::instance().now_us());
    TEST_ASSERT_EQUAL_UINT64(5, VirtualClock::instance().now_ms());
}

void test_scheduled_callback_fires_when_time_reached(void) {
    int fired = 0;
    VirtualClock::instance().schedule_at(100, [&fired]() { fired++; });
    TEST_ASSERT_EQUAL_INT(0, fired);
    VirtualClock::instance().advance_us(50);
    TEST_ASSERT_EQUAL_INT(0, fired);
    VirtualClock::instance().advance_us(50);
    TEST_ASSERT_EQUAL_INT(1, fired);
    VirtualClock::instance().advance_us(1000);  // doesn't fire again
    TEST_ASSERT_EQUAL_INT(1, fired);
}

void test_multiple_callbacks_same_timestamp_fire_in_insertion_order(void) {
    int seq[3] = {0, 0, 0};
    int idx = 0;
    auto& c = VirtualClock::instance();
    c.schedule_at(100, [&]() { seq[idx++] = 1; });
    c.schedule_at(100, [&]() { seq[idx++] = 2; });
    c.schedule_at(100, [&]() { seq[idx++] = 3; });
    c.advance_us(100);
    TEST_ASSERT_EQUAL_INT(1, seq[0]);
    TEST_ASSERT_EQUAL_INT(2, seq[1]);
    TEST_ASSERT_EQUAL_INT(3, seq[2]);
}

void test_cancel_prevents_callback(void) {
    int fired = 0;
    auto h = VirtualClock::instance().schedule_at(100, [&fired]() { fired++; });
    TEST_ASSERT_TRUE(VirtualClock::instance().cancel(h));
    VirtualClock::instance().advance_us(1000);
    TEST_ASSERT_EQUAL_INT(0, fired);
}

void test_cancel_unknown_handle_returns_false(void) {
    TEST_ASSERT_FALSE(VirtualClock::instance().cancel(99999));
}

void test_reset_clears_clock_and_schedule(void) {
    int fired = 0;
    VirtualClock::instance().advance_us(500);
    VirtualClock::instance().schedule_at(1000, [&fired]() { fired++; });
    VirtualClock::instance().reset();
    TEST_ASSERT_EQUAL_UINT64(0, VirtualClock::instance().now_us());
    VirtualClock::instance().advance_us(2000);
    TEST_ASSERT_EQUAL_INT(0, fired);  // scheduled cb cleared by reset
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_clock_starts_at_zero);
    RUN_TEST(test_advance_increments_now_us);
    RUN_TEST(test_advance_ms_converts_correctly);
    RUN_TEST(test_scheduled_callback_fires_when_time_reached);
    RUN_TEST(test_multiple_callbacks_same_timestamp_fire_in_insertion_order);
    RUN_TEST(test_cancel_prevents_callback);
    RUN_TEST(test_cancel_unknown_handle_returns_false);
    RUN_TEST(test_reset_clears_clock_and_schedule);
    return UNITY_END();
}
