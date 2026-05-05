#include <peripherals/FakeDS3231.h>

#include <cstring>

namespace esp32sim::peripherals {

FakeDS3231::FakeDS3231() {
    std::memset(regs_, 0, sizeof(regs_));
    // Reasonable defaults: 2026-01-01 00:00:00, 25.0 C
    setDate(1, 1, 2026);
    setTime(0, 0, 0);
    setTemperature(25.0);
}

void FakeDS3231::setTime(int hour, int minute, int second) {
    regs_[0x00] = bcd_encode(second);
    regs_[0x01] = bcd_encode(minute);
    regs_[0x02] = bcd_encode(hour);  // 24-hour mode (bit 6 = 0)
}

void FakeDS3231::setDate(int day, int month, int year) {
    regs_[0x04] = bcd_encode(day);
    regs_[0x05] = bcd_encode(month);
    // Year register is 0..99 (offset from 2000 in most uses).
    regs_[0x06] = bcd_encode(year % 100);
}

void FakeDS3231::setTemperature(double celsius) {
    // DS3231 temperature: 2-byte signed value, MSB integer, LSB upper 2 bits
    // = .00/.25/.50/.75. We encode positive only for simplicity.
    int t_int = (int)celsius;
    int t_frac_q = (int)((celsius - t_int) * 4) & 0x03;
    regs_[0x11] = (uint8_t)(t_int & 0xFF);
    regs_[0x12] = (uint8_t)(t_frac_q << 6);
}

void FakeDS3231::triggerAlarm1() {
    regs_[0x10] |= 0x01;  // A1F flag in status register
}

uint8_t FakeDS3231::reg(uint8_t addr) const {
    if (addr >= sizeof(regs_)) return 0;
    return regs_[addr];
}

I2CResult FakeDS3231::on_write(const uint8_t* data, size_t len) {
    if (len == 0) return I2CResult::OK;
    // First byte is the register pointer.
    reg_pointer_ = data[0];
    // Remaining bytes (if any) are register writes starting at the pointer
    // (auto-incrementing per DS3231 datasheet).
    for (size_t i = 1; i < len; ++i) {
        if (reg_pointer_ < sizeof(regs_)) {
            regs_[reg_pointer_] = data[i];
        }
        reg_pointer_++;
    }
    return I2CResult::OK;
}

I2CResult FakeDS3231::on_read(uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        if (reg_pointer_ < sizeof(regs_)) {
            buf[i] = regs_[reg_pointer_];
        } else {
            buf[i] = 0;
        }
        reg_pointer_++;
    }
    return I2CResult::OK;
}

}  // namespace esp32sim::peripherals
