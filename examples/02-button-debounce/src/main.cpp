#include <Arduino.h>

constexpr int BUTTON_PIN = 4;
constexpr int LED_PIN = 2;
constexpr unsigned long DEBOUNCE_MS = 50;

int last_stable = HIGH;  // INPUT_PULLUP idle state
int last_raw = HIGH;
unsigned long last_change = 0;

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, last_stable);
}

void loop() {
    int raw = digitalRead(BUTTON_PIN);
    unsigned long now = millis();
    if (raw != last_raw) {
        last_raw = raw;
        last_change = now;
    }
    if ((now - last_change) >= DEBOUNCE_MS && raw != last_stable) {
        last_stable = raw;
        digitalWrite(LED_PIN, last_stable);
    }
}
