// include/esp32sim/pwm.h — LEDC/PWM simulator
#pragma once

#include <cstdint>

namespace esp32sim {

class Pwm {
public:
    static Pwm& instance();
    static constexpr int CHANNEL_COUNT = 16;  // ESP32-S3 has 8; 16 is generous

    void reset();

    // LEDC channel API
    void setup_channel(int ch, uint32_t freq_hz, int resolution_bits);
    void attach_pin(int ch, int pin);
    void set_duty(int ch, uint32_t duty);

    uint32_t duty(int ch) const;
    uint32_t frequency(int ch) const;
    int      resolution(int ch) const;
    int      pin(int ch) const;
    bool     in_use(int ch) const;

private:
    Pwm() = default;
    struct Channel {
        bool in_use = false;
        uint32_t freq_hz = 0;
        int resolution_bits = 0;
        int pin = -1;
        uint32_t duty = 0;
    };
    Channel channels_[CHANNEL_COUNT];
};

}  // namespace esp32sim
