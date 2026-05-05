#include <Wire.h>
#include <esp32sim/i2c.h>
#include <peripherals/FakeDS3231.h>
#include <unity.h>

#include <memory>

using namespace esp32sim;
using esp32sim::peripherals::FakeDS3231;

void setUp(void) { I2CBus::reset_all(); }
void tearDown(void) {}

void test_default_state(void) {
    auto rtc = std::make_shared<FakeDS3231>();
    // Default: 2026-01-01 00:00:00, 25.0 C
    TEST_ASSERT_EQUAL_UINT8(0x00, rtc->reg(0x00));  // seconds (BCD)
    TEST_ASSERT_EQUAL_UINT8(0x00, rtc->reg(0x01));  // minutes
    TEST_ASSERT_EQUAL_UINT8(0x00, rtc->reg(0x02));  // hours
    TEST_ASSERT_EQUAL_UINT8(0x01, rtc->reg(0x04));  // day
    TEST_ASSERT_EQUAL_UINT8(0x01, rtc->reg(0x05));  // month
    TEST_ASSERT_EQUAL_UINT8(0x26, rtc->reg(0x06));  // year (BCD)
    TEST_ASSERT_EQUAL_UINT8(25, rtc->reg(0x11));    // temp integer
}

void test_setTime_BCD_encodes_correctly(void) {
    auto rtc = std::make_shared<FakeDS3231>();
    rtc->setTime(14, 35, 47);
    TEST_ASSERT_EQUAL_UINT8(0x47, rtc->reg(0x00));  // seconds 47
    TEST_ASSERT_EQUAL_UINT8(0x35, rtc->reg(0x01));  // minutes 35
    TEST_ASSERT_EQUAL_UINT8(0x14, rtc->reg(0x02));  // hours 14
}

void test_via_I2C_read_time(void) {
    auto rtc = std::make_shared<FakeDS3231>();
    rtc->setTime(12, 30, 45);
    I2CBus::for_index(0).attach(FakeDS3231::DEFAULT_ADDR, rtc);

    // Real RTClib pattern: write register pointer (0x00), then requestFrom 7 bytes.
    Wire.beginTransmission(0x68);
    Wire.write((uint8_t)0x00);
    Wire.endTransmission(false);  // repeated start
    Wire.requestFrom((uint8_t)0x68, (size_t)7);
    TEST_ASSERT_EQUAL_INT(7, Wire.available());
    uint8_t bytes[7];
    for (int i = 0; i < 7; ++i) bytes[i] = (uint8_t)Wire.read();
    TEST_ASSERT_EQUAL_UINT8(0x45, bytes[0]);  // sec 45
    TEST_ASSERT_EQUAL_UINT8(0x30, bytes[1]);  // min 30
    TEST_ASSERT_EQUAL_UINT8(0x12, bytes[2]);  // hour 12
}

void test_via_I2C_write_time(void) {
    auto rtc = std::make_shared<FakeDS3231>();
    I2CBus::for_index(0).attach(FakeDS3231::DEFAULT_ADDR, rtc);

    // Pattern: write register pointer + 7 time bytes.
    Wire.beginTransmission(0x68);
    Wire.write((uint8_t)0x00);
    Wire.write((uint8_t)0x55);  // sec 55 (BCD)
    Wire.write((uint8_t)0x42);  // min 42 (BCD)
    Wire.write((uint8_t)0x07);  // hr  07 (BCD)
    Wire.endTransmission();

    TEST_ASSERT_EQUAL_UINT8(0x55, rtc->reg(0x00));
    TEST_ASSERT_EQUAL_UINT8(0x42, rtc->reg(0x01));
    TEST_ASSERT_EQUAL_UINT8(0x07, rtc->reg(0x02));
}

void test_setTemperature(void) {
    auto rtc = std::make_shared<FakeDS3231>();
    rtc->setTemperature(22.75);
    TEST_ASSERT_EQUAL_UINT8(22, rtc->reg(0x11));        // integer part
    TEST_ASSERT_EQUAL_UINT8(0x03 << 6, rtc->reg(0x12)); // 0.75 → upper 2 bits = 11
}

void test_alarm_flag_in_status(void) {
    auto rtc = std::make_shared<FakeDS3231>();
    TEST_ASSERT_EQUAL_UINT8(0, rtc->reg(0x10) & 0x01);
    rtc->triggerAlarm1();
    TEST_ASSERT_EQUAL_UINT8(0x01, rtc->reg(0x10) & 0x01);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_default_state);
    RUN_TEST(test_setTime_BCD_encodes_correctly);
    RUN_TEST(test_via_I2C_read_time);
    RUN_TEST(test_via_I2C_write_time);
    RUN_TEST(test_setTemperature);
    RUN_TEST(test_alarm_flag_in_status);
    return UNITY_END();
}
