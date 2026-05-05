#include <Arduino.h>
#include <esp32sim/pwm.h>
#include <esp32sim/strict.h>

#include <cstdio>

extern "C" {

double ledcSetup(uint8_t channel, double freq_hz, uint8_t resolution_bits) {
    if (esp32sim::Strict::instance().enabled() && channel > 7) {
        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "ledcSetup(channel=%d) — ESP32-S3 only has LEDC channels 0..7",
            (int)channel);
        esp32sim::Strict::instance().violation("ESP_SIM_E071", buf);
    }
    esp32sim::Pwm::instance().setup_channel((int)channel, (uint32_t)freq_hz, (int)resolution_bits);
    return freq_hz;
}

void ledcAttachPin(uint8_t pin, uint8_t channel) {
    esp32sim::Pwm::instance().attach_pin((int)channel, (int)pin);
}

void ledcDetachPin(uint8_t /*pin*/) {
    // No-op in T2; real arduino-esp32 disconnects the pin from any channel.
}

void ledcWrite(uint8_t channel, uint32_t duty) {
    if (esp32sim::Strict::instance().enabled() && !esp32sim::Pwm::instance().in_use((int)channel)) {
        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "ledcWrite(channel=%d) without prior ledcSetup — channel not configured",
            (int)channel);
        esp32sim::Strict::instance().violation("ESP_SIM_E070", buf);
    }
    esp32sim::Pwm::instance().set_duty((int)channel, duty);
}

uint32_t ledcRead(uint8_t channel) {
    return esp32sim::Pwm::instance().duty((int)channel);
}

uint32_t ledcReadFreq(uint8_t channel) {
    return esp32sim::Pwm::instance().frequency((int)channel);
}

void analogWrite(uint8_t pin, int val) {
    // Map onto a default channel (0) at 8-bit resolution / 1kHz, similar to
    // arduino-esp32's PWM compatibility shim.
    auto& p = esp32sim::Pwm::instance();
    if (!p.in_use(0)) {
        p.setup_channel(0, 1000, 8);
    }
    p.attach_pin(0, (int)pin);
    p.set_duty(0, (uint32_t)(val < 0 ? 0 : (val > 255 ? 255 : val)));
}

}  // extern "C"
