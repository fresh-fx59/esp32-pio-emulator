// peripherals/mcp23017/include/peripherals/FakeMCP23017.h
//
// Behavioral fake of the Microchip MCP23017 16-bit I/O expander. Default
// address 0x20. BANK=0 register addressing (default after power-on; both
// banks paired at consecutive addresses).
//
// Registers (BANK=0):
//   0x00 IODIRA, 0x01 IODIRB  — direction (1=input, 0=output)
//   0x02 IPOLA,  0x03 IPOLB   — input polarity
//   0x04 GPINTENA, 0x05 GPINTENB — interrupt-on-change enable
//   0x06 DEFVALA, 0x07 DEFVALB
//   0x08 INTCONA, 0x09 INTCONB
//   0x0A IOCON,  0x0B IOCON (mirror)
//   0x0C GPPUA, 0x0D GPPUB    — pullups
//   0x0E INTFA, 0x0F INTFB
//   0x10 INTCAPA, 0x11 INTCAPB
//   0x12 GPIOA,  0x13 GPIOB   — pin levels (read = current input; write = output)
//   0x14 OLATA, 0x15 OLATB    — output latch
#pragma once

#include <esp32sim/i2c.h>

#include <cstdint>

namespace esp32sim::peripherals {

class FakeMCP23017 : public esp32sim::I2CDevice {
public:
    static constexpr uint8_t DEFAULT_ADDR = 0x20;

    FakeMCP23017();

    // Test-side: drive a pin's input level (0..15, where 0..7 = port A, 8..15 = port B).
    void setPin(int pin, int level);
    int  getPin(int pin) const;
    void setMode(int pin, bool input);

    I2CResult on_write(const uint8_t* data, size_t len) override;
    I2CResult on_read(uint8_t* buf, size_t len) override;

    uint8_t reg(uint8_t addr) const { return regs_[addr]; }

private:
    uint8_t regs_[0x16] = {};
    uint8_t reg_pointer_ = 0;
};

}  // namespace esp32sim::peripherals
