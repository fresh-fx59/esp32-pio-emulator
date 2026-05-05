#include <esp32sim/clock.h>
#include <esp32sim/event_log.h>
#include <unity.h>

using esp32sim::Event;
using esp32sim::EventKind;
using esp32sim::EventLog;
using esp32sim::VirtualClock;

void setUp(void) {
    VirtualClock::instance().reset();
    EventLog::instance().reset();
}
void tearDown(void) {}

void test_log_starts_empty(void) {
    TEST_ASSERT_EQUAL_size_t(0, EventLog::instance().count());
}

void test_emit_appends_event_with_current_timestamp(void) {
    VirtualClock::instance().advance_us(500);
    EventLog::instance().emit(Event{EventKind::GPIO_WRITE, /*pin*/2, /*value*/1});
    TEST_ASSERT_EQUAL_size_t(1, EventLog::instance().count());
    auto evs = EventLog::instance().all();
    TEST_ASSERT_EQUAL_UINT64(500, evs[0].timestamp_us);
    TEST_ASSERT_EQUAL_INT((int)EventKind::GPIO_WRITE, (int)evs[0].kind);
    TEST_ASSERT_EQUAL_INT(2, evs[0].pin);
    TEST_ASSERT_EQUAL_INT(1, evs[0].value);
}

void test_filter_by_kind(void) {
    auto& log = EventLog::instance();
    log.emit(Event{EventKind::GPIO_WRITE, 2, 1});
    log.emit(Event{EventKind::UART_TX, 0, 'h'});
    log.emit(Event{EventKind::GPIO_WRITE, 2, 0});
    auto gpio = log.filter([](const Event& e) { return e.kind == EventKind::GPIO_WRITE; });
    TEST_ASSERT_EQUAL_size_t(2, gpio.size());
}

void test_filter_by_time_range(void) {
    auto& log = EventLog::instance();
    auto& clk = VirtualClock::instance();
    log.emit(Event{EventKind::GPIO_WRITE, 1, 1});  // t=0
    clk.advance_us(100);
    log.emit(Event{EventKind::GPIO_WRITE, 2, 1});  // t=100
    clk.advance_us(100);
    log.emit(Event{EventKind::GPIO_WRITE, 3, 1});  // t=200
    auto in_range = log.between(50, 150);
    TEST_ASSERT_EQUAL_size_t(1, in_range.size());
    TEST_ASSERT_EQUAL_INT(2, in_range[0].pin);
}

void test_reset_clears_log(void) {
    EventLog::instance().emit(Event{EventKind::GPIO_WRITE, 2, 1});
    EventLog::instance().reset();
    TEST_ASSERT_EQUAL_size_t(0, EventLog::instance().count());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_log_starts_empty);
    RUN_TEST(test_emit_appends_event_with_current_timestamp);
    RUN_TEST(test_filter_by_kind);
    RUN_TEST(test_filter_by_time_range);
    RUN_TEST(test_reset_clears_log);
    return UNITY_END();
}
