#include <Arduino.h>

void setup() {
    Serial.begin(115200);
}

void loop() {
    while (Serial.available()) {
        int c = Serial.read();
        if (c >= 'a' && c <= 'z') c -= 32;
        Serial.write((uint8_t)c);
    }
}
