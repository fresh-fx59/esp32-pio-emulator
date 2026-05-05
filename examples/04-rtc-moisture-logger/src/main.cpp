// examples/04-rtc-moisture-logger/src/main.cpp
//
// Reads time from a DS3231 RTC over I2C, reads moisture sensor on ADC pin 34,
// and logs every 10 seconds via Serial. Demonstrates I2C + ADC + Serial
// + virtual time, plus the load-bearing T2 acceptance: this same sketch is
// driven by both Unity (in-process) and pytest-embedded (out-of-process).
#include <Arduino.h>
#include <Wire.h>

constexpr uint8_t RTC_ADDR = 0x68;
constexpr int MOISTURE_PIN = 34;
constexpr unsigned long INTERVAL_MS = 10000;

unsigned long last_log = 0;
bool first_iteration = true;

static int bcd_to_int(uint8_t b) { return ((b >> 4) * 10) + (b & 0x0F); }

void setup() {
    Serial.begin(115200);
    Wire.begin();
    Serial.println("READY");
}

void loop() {
    unsigned long now = millis();
    if (!first_iteration && (now - last_log < INTERVAL_MS)) return;
    first_iteration = false;
    last_log = now;

    // Read 7 bytes starting at register 0x00 (sec, min, hr, dow, day, mon, yr).
    Wire.beginTransmission(RTC_ADDR);
    Wire.write((uint8_t)0x00);
    Wire.endTransmission(false);
    Wire.requestFrom(RTC_ADDR, (size_t)7);
    int sec = bcd_to_int(Wire.read());
    int mn  = bcd_to_int(Wire.read());
    int hr  = bcd_to_int(Wire.read());
    Wire.read();  // dow (unused)
    int day = bcd_to_int(Wire.read());
    int mon = bcd_to_int(Wire.read());
    int yr  = bcd_to_int(Wire.read()) + 2000;

    int moisture = analogRead(MOISTURE_PIN);

    Serial.printf("%04d-%02d-%02d %02d:%02d:%02d moisture=%d\n",
                  yr, mon, day, hr, mn, sec, moisture);
}
