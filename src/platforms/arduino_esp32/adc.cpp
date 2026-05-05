#include <Arduino.h>
#include <esp32sim/adc.h>
#include <esp32sim/strict.h>

#include <cstdio>

namespace {
bool is_adc_pin_s3(int pin) {
    return pin >= 1 && pin <= 20;  // ADC1: 1-10, ADC2: 11-20 on ESP32-S3
}
}  // namespace

extern "C" {

int analogRead(uint8_t pin) {
    if (esp32sim::Strict::instance().enabled() && !is_adc_pin_s3((int)pin)) {
        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "analogRead on GPIO %d which is not ADC-capable on ESP32-S3 "
            "(ADC1: GPIO 1-10, ADC2: GPIO 11-20)",
            (int)pin);
        esp32sim::Strict::instance().violation("ESP_SIM_E004", buf, esp32sim::Severity::WARNING);
    }
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
