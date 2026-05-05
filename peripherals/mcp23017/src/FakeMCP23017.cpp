#include <peripherals/FakeMCP23017.h>

#include <cstring>

namespace esp32sim::peripherals {

FakeMCP23017::FakeMCP23017() {
    std::memset(regs_, 0, sizeof(regs_));
    // Power-on defaults per datasheet: all pins are inputs (IODIR=0xFF).
    regs_[0x00] = 0xFF;  // IODIRA
    regs_[0x01] = 0xFF;  // IODIRB
}

void FakeMCP23017::setPin(int pin, int level) {
    if (pin < 0 || pin > 15) return;
    int port = pin / 8;        // 0 = A, 1 = B
    int bit  = pin % 8;
    uint8_t& gpio_reg = (port == 0) ? regs_[0x12] : regs_[0x13];
    if (level) gpio_reg |= (uint8_t)(1 << bit);
    else       gpio_reg &= (uint8_t)~(1 << bit);
}

int FakeMCP23017::getPin(int pin) const {
    if (pin < 0 || pin > 15) return 0;
    int port = pin / 8;
    int bit  = pin % 8;
    uint8_t r = (port == 0) ? regs_[0x12] : regs_[0x13];
    return (r >> bit) & 0x01;
}

void FakeMCP23017::setMode(int pin, bool input) {
    if (pin < 0 || pin > 15) return;
    int port = pin / 8;
    int bit  = pin % 8;
    uint8_t& iodir = (port == 0) ? regs_[0x00] : regs_[0x01];
    if (input) iodir |= (uint8_t)(1 << bit);
    else       iodir &= (uint8_t)~(1 << bit);
}

I2CResult FakeMCP23017::on_write(const uint8_t* data, size_t len) {
    if (len == 0) return I2CResult::OK;
    reg_pointer_ = data[0];
    for (size_t i = 1; i < len; ++i) {
        if (reg_pointer_ < sizeof(regs_)) {
            regs_[reg_pointer_] = data[i];
            // Writing GPIOA/GPIOB also updates OLATA/OLATB per datasheet.
            if (reg_pointer_ == 0x12) regs_[0x14] = data[i];
            if (reg_pointer_ == 0x13) regs_[0x15] = data[i];
        }
        reg_pointer_++;
    }
    return I2CResult::OK;
}

I2CResult FakeMCP23017::on_read(uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        buf[i] = (reg_pointer_ < sizeof(regs_)) ? regs_[reg_pointer_] : 0;
        reg_pointer_++;
    }
    return I2CResult::OK;
}

}  // namespace esp32sim::peripherals
