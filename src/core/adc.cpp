#include <esp32sim/adc.h>

namespace esp32sim {

Adc& Adc::instance() {
    static Adc a;
    return a;
}

void Adc::reset() {
    for (int& v : values_) v = 0;
    for (int& a : attens_) a = 0;
    resolution_bits_ = 12;
}

void Adc::set_value(int pin, int raw_value) {
    if (pin < 0 || pin > MAX_PIN) return;
    values_[pin] = raw_value;
}

int Adc::get_value(int pin) const {
    if (pin < 0 || pin > MAX_PIN) return 0;
    return values_[pin];
}

void Adc::set_attenuation(int pin, int atten) {
    if (pin < 0 || pin > MAX_PIN) return;
    attens_[pin] = atten;
}

int Adc::attenuation(int pin) const {
    if (pin < 0 || pin > MAX_PIN) return 0;
    return attens_[pin];
}

}  // namespace esp32sim
