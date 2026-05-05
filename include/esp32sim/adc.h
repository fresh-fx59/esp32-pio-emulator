// include/esp32sim/adc.h — ADC simulator
#pragma once

#include <cstdint>

namespace esp32sim {

class Adc {
public:
    static Adc& instance();
    static constexpr int MAX_PIN = 63;

    void reset();

    // Test-side: drive a value into a pin's ADC reading.
    void set_value(int pin, int raw_value);
    int  get_value(int pin) const;

    // Resolution affects nothing in T2's model directly (we just store the raw
    // value) but is recorded so analogRead can clamp later. Default 12 bits
    // (4095 max) on ESP32-S3.
    void set_resolution(int bits) { resolution_bits_ = bits; }
    int  resolution() const { return resolution_bits_; }

    // Attenuation is recorded per-pin for completeness; not modeled in T2.
    void set_attenuation(int pin, int atten);
    int  attenuation(int pin) const;

private:
    Adc() { reset(); }
    int values_[MAX_PIN + 1] = {};
    int attens_[MAX_PIN + 1] = {};
    int resolution_bits_ = 12;
};

}  // namespace esp32sim
