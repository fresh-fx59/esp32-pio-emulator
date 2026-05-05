// peripherals/ds3231/include/peripherals/FakeDS3231.h
//
// Behavioral fake of the Maxim DS3231 precision I2C RTC. Default address
// 0x68. Implements the subset of registers that arduino-esp32 RTC libraries
// (RTClib::DS3231, Sodaq_DS3231, etc.) actually touch: time/date (00-06),
// alarms (07-0E), control (0F), status (10), aging offset (11), temperature
// (11-12).
//
// Times are stored in BCD per the datasheet. The fake's setTime API takes
// regular ints and encodes/decodes for you.
#pragma once

#include <esp32sim/i2c.h>

#include <cstdint>

namespace esp32sim::peripherals {

class FakeDS3231 : public esp32sim::I2CDevice {
public:
    static constexpr uint8_t DEFAULT_ADDR = 0x68;

    FakeDS3231();

    // Test-side time setters (regular base-10 ints; we BCD-encode internally)
    void setTime(int hour, int minute, int second);
    void setDate(int day, int month, int year);  // year is full e.g. 2026
    void setTemperature(double celsius);
    void triggerAlarm1();  // sets the alarm flag in the status register

    // I2CDevice interface
    I2CResult on_write(const uint8_t* data, size_t len) override;
    I2CResult on_read(uint8_t* buf, size_t len) override;

    // Inspection (for tests asserting on register state)
    uint8_t reg(uint8_t addr) const;

private:
    static uint8_t bcd_encode(int v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }
    static int     bcd_decode(uint8_t b) { return ((b >> 4) * 10) + (b & 0x0F); }

    uint8_t regs_[0x13] = {};   // 0x00..0x12 inclusive
    uint8_t reg_pointer_ = 0;
};

}  // namespace esp32sim::peripherals
