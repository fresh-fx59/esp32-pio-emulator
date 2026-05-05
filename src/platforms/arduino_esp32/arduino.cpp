// src/platforms/arduino_esp32/arduino.cpp
//
// Implementation of the Arduino.h API in terms of the framework core.
#include <Arduino.h>
#include <esp32sim/clock.h>
#include <esp32sim/gpio.h>

namespace {
esp32sim::PinMode translate_mode(uint8_t arduino_mode) {
    // Arduino.h macro values → core PinMode enum.
    switch (arduino_mode) {
        case OUTPUT:         return esp32sim::PinMode::OUTPUT;
        case INPUT_PULLUP:   return esp32sim::PinMode::INPUT_PULLUP;
        case INPUT_PULLDOWN: return esp32sim::PinMode::INPUT_PULLDOWN;
        case INPUT:
        default:             return esp32sim::PinMode::INPUT;
    }
}
}  // namespace

extern "C" {

void pinMode(uint8_t pin, uint8_t mode) {
    esp32sim::PinRegistry::instance().set_mode((int)pin, translate_mode(mode));
}

void digitalWrite(uint8_t pin, uint8_t val) {
    esp32sim::PinRegistry::instance().set_level((int)pin, val ? 1 : 0);
}

int digitalRead(uint8_t pin) {
    return esp32sim::PinRegistry::instance().get_level((int)pin);
}

uint32_t millis(void) {
    return (uint32_t)esp32sim::VirtualClock::instance().now_ms();
}

uint32_t micros(void) {
    return (uint32_t)esp32sim::VirtualClock::instance().now_us();
}

void delay(uint32_t ms) {
    esp32sim::VirtualClock::instance().advance_ms(ms);
}

void delayMicroseconds(uint32_t us) {
    esp32sim::VirtualClock::instance().advance_us(us);
}

void yield(void) {
    // T1 no-op. T4: signal scheduler if FreeRTOS shim is active.
}

}  // extern "C"
