// examples/05-pwm-fade/src/main.cpp
//
// LED fade demo using LEDC. Channel 0, 8-bit resolution, 1kHz, attached to
// pin 5. Fades 0 → 255 → 0 over ~5.1 seconds (10ms per step).
#include <Arduino.h>

constexpr int LED_PIN = 5;
constexpr int LED_CHANNEL = 0;
constexpr int FADE_STEP_MS = 10;

int duty = 0;
int dir = 1;
unsigned long last_step = 0;

void setup() {
    ledcSetup(LED_CHANNEL, 1000, 8);
    ledcAttachPin(LED_PIN, LED_CHANNEL);
    ledcWrite(LED_CHANNEL, 0);
}

void loop() {
    unsigned long now = millis();
    if (now - last_step < FADE_STEP_MS) return;
    last_step = now;

    duty += dir;
    if (duty >= 255) { duty = 255; dir = -1; }
    if (duty <= 0)   { duty = 0;   dir = 1; }
    ledcWrite(LED_CHANNEL, (uint32_t)duty);
}
