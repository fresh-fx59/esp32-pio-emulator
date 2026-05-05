// peripherals/bmp280/include/peripherals/FakeBMP280.h
//
// Behavioral fake of the Bosch BMP280 pressure + temperature sensor over I2C.
// Default address 0x76 (alternate 0x77). Register layout matches the BMP280
// datasheet:
//   0xD0: chip ID = 0x58
//   0x88-0xA1: calibration coefficients (T1, T2, T3, P1..P9)
//   0xF4: ctrl_meas (oversampling + power mode)
//   0xF5: config
//   0xF7-0xF9: pressure raw (24-bit MSB first)
//   0xFA-0xFC: temp raw (24-bit MSB first)
//
// We expose setTemperature(C) and setPressure(Pa). These set the raw
// registers using *fixed* synthetic calibration coefficients chosen so the
// Bosch back-conversion formulas (used by Adafruit_BMP280, etc.) yield the
// requested inputs. T2 minimum-viable: nominal coefficients, single-precision
// approximation; if a real library exposes a small reading mismatch, we'll
// tighten the math in T2.5.
#pragma once

#include <esp32sim/i2c.h>

#include <cstdint>

namespace esp32sim::peripherals {

class FakeBMP280 : public esp32sim::I2CDevice {
public:
    static constexpr uint8_t DEFAULT_ADDR = 0x76;

    FakeBMP280();

    // Test-side convenience setters
    void setTemperature(double celsius);
    void setPressure(double pascals);

    // I2CDevice
    I2CResult on_write(const uint8_t* data, size_t len) override;
    I2CResult on_read(uint8_t* buf, size_t len) override;

    uint8_t reg(uint8_t addr) const;

private:
    // BMP280 datasheet calibration coefficient layout (16-bit values stored
    // little-endian over registers 0x88..0xA1).
    void install_default_calibration();
    void update_temp_registers(double celsius);
    void update_pressure_registers(double pascals);

    // Full register space 0x00-0xFF (sparse; we use direct addressing).
    uint8_t regs_[0x100] = {};
    uint8_t reg_pointer_ = 0;

    // Cached calibration values used in the back-conversion math.
    uint16_t dig_T1_ = 0;
    int16_t  dig_T2_ = 0, dig_T3_ = 0;
    uint16_t dig_P1_ = 0;
    int16_t  dig_P2_ = 0, dig_P3_ = 0, dig_P4_ = 0, dig_P5_ = 0,
             dig_P6_ = 0, dig_P7_ = 0, dig_P8_ = 0, dig_P9_ = 0;
};

}  // namespace esp32sim::peripherals
