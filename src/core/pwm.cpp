#include <esp32sim/event_log.h>
#include <esp32sim/pwm.h>

namespace esp32sim {

Pwm& Pwm::instance() {
    static Pwm p;
    return p;
}

void Pwm::reset() {
    for (auto& c : channels_) c = Channel{};
}

void Pwm::setup_channel(int ch, uint32_t freq_hz, int resolution_bits) {
    if (ch < 0 || ch >= CHANNEL_COUNT) return;
    channels_[ch].in_use = true;
    channels_[ch].freq_hz = freq_hz;
    channels_[ch].resolution_bits = resolution_bits;
}

void Pwm::attach_pin(int ch, int pin) {
    if (ch < 0 || ch >= CHANNEL_COUNT) return;
    channels_[ch].pin = pin;
}

void Pwm::set_duty(int ch, uint32_t duty) {
    if (ch < 0 || ch >= CHANNEL_COUNT) return;
    channels_[ch].duty = duty;
}

uint32_t Pwm::duty(int ch) const {
    if (ch < 0 || ch >= CHANNEL_COUNT) return 0;
    return channels_[ch].duty;
}

uint32_t Pwm::frequency(int ch) const {
    if (ch < 0 || ch >= CHANNEL_COUNT) return 0;
    return channels_[ch].freq_hz;
}

int Pwm::resolution(int ch) const {
    if (ch < 0 || ch >= CHANNEL_COUNT) return 0;
    return channels_[ch].resolution_bits;
}

int Pwm::pin(int ch) const {
    if (ch < 0 || ch >= CHANNEL_COUNT) return -1;
    return channels_[ch].pin;
}

bool Pwm::in_use(int ch) const {
    if (ch < 0 || ch >= CHANNEL_COUNT) return false;
    return channels_[ch].in_use;
}

}  // namespace esp32sim
