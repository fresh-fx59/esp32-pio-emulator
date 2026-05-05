#include <Arduino.h>
#include <HardwareSerial.h>
#include <esp32sim/clock.h>
#include <esp32sim/event_log.h>
#include <esp32sim/uart.h>
#include <unity.h>

void setUp(void) {
    esp32sim::VirtualClock::instance().reset();
    esp32sim::EventLog::instance().reset();
    esp32sim::UartChannel::reset_all();
}
void tearDown(void) {}

void test_serial_println_appends_crlf(void) {
    Serial.begin(115200);
    Serial.println("hello");
    auto& u = esp32sim::UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_STRING("hello\r\n", u.drain_tx().c_str());
}

void test_serial_print_no_newline(void) {
    Serial.print("abc");
    auto& u = esp32sim::UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_STRING("abc", u.drain_tx().c_str());
}

void test_serial_printf(void) {
    Serial.printf("x=%d y=%d", 1, 42);
    auto& u = esp32sim::UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_STRING("x=1 y=42", u.drain_tx().c_str());
}

void test_serial_print_int(void) {
    Serial.print(123);
    auto& u = esp32sim::UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_STRING("123", u.drain_tx().c_str());
}

void test_serial_print_int_with_base(void) {
    Serial.print(255, 16);  // hex
    auto& u = esp32sim::UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_STRING("ff", u.drain_tx().c_str());
}

void test_serial_write_single_byte(void) {
    Serial.write('Q');
    auto& u = esp32sim::UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_STRING("Q", u.drain_tx().c_str());
}

void test_serial_read_consumes_rx(void) {
    auto& u = esp32sim::UartChannel::for_index(0);
    u.rx_inject("hi");
    TEST_ASSERT_TRUE(Serial.available());
    TEST_ASSERT_EQUAL_INT('h', Serial.read());
    TEST_ASSERT_EQUAL_INT('i', Serial.read());
    TEST_ASSERT_EQUAL_INT(-1, Serial.read());
    TEST_ASSERT_FALSE(Serial.available());
}

void test_serial_peek_does_not_consume(void) {
    auto& u = esp32sim::UartChannel::for_index(0);
    u.rx_inject("abc");
    TEST_ASSERT_EQUAL_INT('a', Serial.peek());
    TEST_ASSERT_EQUAL_INT('a', Serial.peek());
    TEST_ASSERT_EQUAL_INT(3, Serial.available());
}

void test_serial1_serial2_isolated(void) {
    Serial.print("s0");
    Serial1.print("s1");
    Serial2.print("s2");
    TEST_ASSERT_EQUAL_STRING("s0", esp32sim::UartChannel::for_index(0).drain_tx().c_str());
    TEST_ASSERT_EQUAL_STRING("s1", esp32sim::UartChannel::for_index(1).drain_tx().c_str());
    TEST_ASSERT_EQUAL_STRING("s2", esp32sim::UartChannel::for_index(2).drain_tx().c_str());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_serial_println_appends_crlf);
    RUN_TEST(test_serial_print_no_newline);
    RUN_TEST(test_serial_printf);
    RUN_TEST(test_serial_print_int);
    RUN_TEST(test_serial_print_int_with_base);
    RUN_TEST(test_serial_write_single_byte);
    RUN_TEST(test_serial_read_consumes_rx);
    RUN_TEST(test_serial_peek_does_not_consume);
    RUN_TEST(test_serial1_serial2_isolated);
    return UNITY_END();
}
