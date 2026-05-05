#include <Arduino.h>

constexpr int LED_PIN = 2;
constexpr unsigned long PERIOD_MS = 500;
unsigned long last_toggle = 0;
bool led_on = false;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void loop() {
    unsigned long now = millis();
    if (now - last_toggle >= PERIOD_MS) {
        led_on = !led_on;
        digitalWrite(LED_PIN, led_on ? HIGH : LOW);
        last_toggle = now;
    }
}
