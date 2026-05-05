#include <Arduino.h>
#include <esp32sim/adc.h>

extern "C" {

int analogRead(uint8_t pin) {
    return esp32sim::Adc::instance().get_value((int)pin);
}

void analogReadResolution(uint8_t bits) {
    esp32sim::Adc::instance().set_resolution((int)bits);
}

void analogSetAttenuation(int atten) {
    auto& a = esp32sim::Adc::instance();
    for (int p = 0; p <= esp32sim::Adc::MAX_PIN; ++p) a.set_attenuation(p, atten);
}

void analogSetPinAttenuation(uint8_t pin, int atten) {
    esp32sim::Adc::instance().set_attenuation((int)pin, atten);
}

}  // extern "C"
