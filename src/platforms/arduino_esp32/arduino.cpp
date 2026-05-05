// src/platforms/arduino_esp32/arduino.cpp
//
// Implementation of the Arduino.h API in terms of the framework core.
#include <Arduino.h>
#include <esp32sim/adc.h>
#include <esp32sim/clock.h>
#include <esp32sim/gpio.h>
#include <esp32sim/strict.h>

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

// ESP32-S3 chip-contract rules. (See docs/user/reference/strict-mode.md.)
constexpr int kEsp32S3MaxPin = 48;

bool is_flash_pin_s3(int pin) {
    // GPIO 26-32 are SPI flash on ESP32-S3 (see ESP32-S3 datasheet §3.4).
    // GPIO 33-37 may also be reserved when octal-flash variants are used; we
    // flag those as a softer warning later if user demand surfaces.
    return pin >= 26 && pin <= 32;
}

bool is_strapping_pin_s3(int pin) {
    return pin == 0 || pin == 3 || pin == 45 || pin == 46;
}

bool is_adc_pin_s3(int pin) {
    // ADC1 = GPIO 1-10, ADC2 = GPIO 11-20 on ESP32-S3.
    return pin >= 1 && pin <= 20;
}

bool is_usb_jtag_pin_s3(int pin) {
    return pin == 19 || pin == 20;
}

void check_pin_in_range(int pin, const char* api) {
    if (pin < 0 || pin > kEsp32S3MaxPin) {
        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "%s called on pin %d which is out of range (ESP32-S3 max GPIO is %d)",
            api, pin, kEsp32S3MaxPin);
        esp32sim::Strict::instance().violation("ESP_SIM_E003", buf);
    }
}

void check_flash_pin(int pin, const char* api) {
    if (is_flash_pin_s3(pin)) {
        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "%s on GPIO %d which is reserved for SPI flash on ESP32-S3 — "
            "using it will corrupt flash and brick the device",
            api, pin);
        esp32sim::Strict::instance().violation("ESP_SIM_E002", buf);
    }
}

}  // namespace

extern "C" {

void pinMode(uint8_t pin, uint8_t mode) {
    if (esp32sim::Strict::instance().enabled()) {
        check_pin_in_range((int)pin, "pinMode");
        check_flash_pin((int)pin, "pinMode");
        if (is_strapping_pin_s3((int)pin)) {
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                "pinMode on GPIO %d which is a strapping pin on ESP32-S3 — "
                "may interfere with boot/flash mode selection",
                (int)pin);
            esp32sim::Strict::instance().violation("ESP_SIM_E006", buf);
        }
    }
    esp32sim::PinRegistry::instance().set_mode((int)pin, translate_mode(mode));
}

void digitalWrite(uint8_t pin, uint8_t val) {
    auto& strict = esp32sim::Strict::instance();
    if (strict.enabled()) {
        check_pin_in_range((int)pin, "digitalWrite");
        check_flash_pin((int)pin, "digitalWrite");
        // ESP_SIM_E001: digitalWrite without prior pinMode(OUTPUT)
        auto m = esp32sim::PinRegistry::instance().get_mode((int)pin);
        if (m != esp32sim::PinMode::OUTPUT && m != esp32sim::PinMode::OUTPUT_OPEN_DRAIN) {
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                "digitalWrite on GPIO %d with mode != OUTPUT — "
                "must call pinMode(%d, OUTPUT) before digitalWrite",
                (int)pin, (int)pin);
            strict.violation("ESP_SIM_E001", buf);
        }
    }
    esp32sim::PinRegistry::instance().set_level((int)pin, val ? 1 : 0);
}

int digitalRead(uint8_t pin) {
    if (esp32sim::Strict::instance().enabled()) {
        check_pin_in_range((int)pin, "digitalRead");
        check_flash_pin((int)pin, "digitalRead");
    }
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
    if (esp32sim::Strict::instance().enabled() && us > 16383) {
        char buf[160];
        std::snprintf(buf, sizeof(buf),
            "delayMicroseconds(%u) exceeds the 16383 hardware maximum on ESP32 — "
            "values above this are treated as zero on real hardware",
            us);
        esp32sim::Strict::instance().violation("ESP_SIM_E040", buf);
    }
    esp32sim::VirtualClock::instance().advance_us(us);
}

void yield(void) {
    // T1 no-op. T4: signal scheduler if FreeRTOS shim is active.
}

}  // extern "C"
