// examples/07-deep-sleep-mqtt — T4 reference: NVS + WiFi + MQTT + deep sleep
//
// On boot:
//   1. Read WiFi credentials from NVS. If absent, advertise BLE provisioning
//      and stop (a real sketch would wait; here we just stop).
//   2. Otherwise: WiFi.begin → MQTT.connect → publish a temperature reading
//      → enter deep sleep for 60 seconds.
//   3. Boot count tracked in NVS; published as part of the message.
//
// Demonstrates: Preferences (NVS), WiFi.begin, PubSubClient.publish,
// esp_deep_sleep_start, BLEDevice.init, all together.
#include <Arduino.h>
#include <BLEDevice.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <esp_sleep.h>

#include <string>

constexpr uint64_t SLEEP_US = 60ULL * 1000ULL * 1000ULL;  // 60 s

PubSubClient mqtt;

void setup() {
    Serial.begin(115200);
    Serial.println("BOOT");

    Preferences prefs;
    prefs.begin("config");
    uint32_t boot_count = prefs.getUInt("boots", 0) + 1;
    prefs.putUInt("boots", boot_count);
    Serial.printf("BOOT_COUNT %u\n", boot_count);

    std::string ssid = prefs.getString("ssid").c_str();
    if (ssid.empty()) {
        Serial.println("NO_CREDS");
        BLEDevice::init("esp32-provision");
        BLEDevice::createServer();
        BLEDevice::getAdvertising()->start();
        Serial.println("BLE_PROVISIONING");
        prefs.end();
        return;
    }
    std::string pass = prefs.getString("pass").c_str();
    prefs.end();

    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.printf("WIFI %s\n", WiFi.SSID());

    mqtt.setServer("broker.example.com", 1883);
    mqtt.connect("device-1");

    char payload[64];
    snprintf(payload, sizeof(payload), "{\"boot\":%u,\"temp\":22.5}", boot_count);
    mqtt.publish("telemetry/sensor", payload);
    Serial.printf("PUBLISHED %s\n", payload);

    esp_sleep_enable_timer_wakeup(SLEEP_US);
    Serial.println("SLEEPING");
    esp_deep_sleep_start();
}

void loop() {
    // Unreachable in normal flow (deep sleep restarts via setup).
}
