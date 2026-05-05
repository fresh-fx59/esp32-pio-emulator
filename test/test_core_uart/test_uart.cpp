#include <esp32sim/clock.h>
#include <esp32sim/event_log.h>
#include <esp32sim/uart.h>
#include <unity.h>

using namespace esp32sim;

void setUp(void) {
    VirtualClock::instance().reset();
    EventLog::instance().reset();
    UartChannel::reset_all();
}
void tearDown(void) {}

void test_uart0_starts_empty(void) {
    auto& u = UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_size_t(0, u.tx_buffer());
    TEST_ASSERT_FALSE(u.rx_available());
}

void test_tx_write_appends_and_emits_event(void) {
    auto& u = UartChannel::for_index(0);
    u.tx_write_byte('H');
    u.tx_write_byte('i');
    TEST_ASSERT_EQUAL_STRING("Hi", u.drain_tx().c_str());
    auto txes = EventLog::instance().by_kind(EventKind::UART_TX);
    TEST_ASSERT_EQUAL_size_t(2, txes.size());
}

void test_drain_tx_clears_buffer(void) {
    auto& u = UartChannel::for_index(0);
    u.tx_write_byte('a');
    TEST_ASSERT_EQUAL_STRING("a", u.drain_tx().c_str());
    TEST_ASSERT_EQUAL_STRING("", u.drain_tx().c_str());
}

void test_tx_history_preserves_full_log(void) {
    auto& u = UartChannel::for_index(0);
    u.tx_write_byte('a');
    u.drain_tx();
    u.tx_write_byte('b');
    TEST_ASSERT_EQUAL_STRING("ab", u.tx_history().c_str());
}

void test_rx_inject_then_read(void) {
    auto& u = UartChannel::for_index(0);
    u.rx_inject("hello");
    TEST_ASSERT_TRUE(u.rx_available());
    TEST_ASSERT_EQUAL_INT(5, u.rx_size());
    TEST_ASSERT_EQUAL_INT('h', u.rx_read_byte());
    TEST_ASSERT_EQUAL_INT('e', u.rx_read_byte());
    TEST_ASSERT_EQUAL_INT(3, u.rx_size());
}

void test_rx_read_when_empty_returns_minus_one(void) {
    auto& u = UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_INT(-1, u.rx_read_byte());
}

void test_rx_peek_does_not_consume(void) {
    auto& u = UartChannel::for_index(0);
    u.rx_inject("abc");
    TEST_ASSERT_EQUAL_INT('a', u.rx_peek());
    TEST_ASSERT_EQUAL_INT('a', u.rx_peek());
    TEST_ASSERT_EQUAL_INT(3, u.rx_size());
}

void test_three_uarts_isolated(void) {
    auto& u0 = UartChannel::for_index(0);
    auto& u1 = UartChannel::for_index(1);
    auto& u2 = UartChannel::for_index(2);
    u0.tx_write_byte('a');
    u1.tx_write_byte('b');
    u2.tx_write_byte('c');
    TEST_ASSERT_EQUAL_STRING("a", u0.drain_tx().c_str());
    TEST_ASSERT_EQUAL_STRING("b", u1.drain_tx().c_str());
    TEST_ASSERT_EQUAL_STRING("c", u2.drain_tx().c_str());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_uart0_starts_empty);
    RUN_TEST(test_tx_write_appends_and_emits_event);
    RUN_TEST(test_drain_tx_clears_buffer);
    RUN_TEST(test_tx_history_preserves_full_log);
    RUN_TEST(test_rx_inject_then_read);
    RUN_TEST(test_rx_read_when_empty_returns_minus_one);
    RUN_TEST(test_rx_peek_does_not_consume);
    RUN_TEST(test_three_uarts_isolated);
    return UNITY_END();
}
