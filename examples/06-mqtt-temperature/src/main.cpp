// examples/06-mqtt-temperature/src/main.cpp
//
// Temperature logger that connects to WiFi, fetches a config from HTTP,
// then publishes BMP280 readings via MQTT. Demonstrates the full T3
// networking surface against test-driven mocks.
#include <Arduino.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>

#include <string>

constexpr unsigned long PUBLISH_INTERVAL_MS = 5000;
unsigned long last_publish = 0;
bool initialized = false;
std::string mqtt_topic = "sensors/temp";

PubSubClient mqtt;

void setup() {
    Serial.begin(115200);
    Wire.begin();

    Serial.println("BOOT");
    WiFi.begin("HomeNetwork", "secret");
    Serial.printf("WIFI %s %s\n", WiFi.SSID(), WiFi.localIP().toString().c_str());

    HTTPClient http;
    http.begin("https://config.example.com/topic");
    if (http.GET() == 200) {
        mqtt_topic = http.getString().c_str();
    }
    http.end();
    Serial.printf("TOPIC %s\n", mqtt_topic.c_str());

    mqtt.setServer("broker.example.com", 1883);
    mqtt.connect("temp-logger");
    Serial.println("MQTT_CONNECTED");
    initialized = true;
}

void loop() {
    if (!initialized) return;
    mqtt.loop();
    unsigned long now = millis();
    if (now - last_publish < PUBLISH_INTERVAL_MS) return;
    last_publish = now;

    // Pretend BMP280 reading — in a real sketch this would be Adafruit_BMP280
    // or similar. For T3 demo we just publish a fixed value.
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", 22.50);
    mqtt.publish(mqtt_topic.c_str(), buf);
    Serial.printf("PUBLISHED %s\n", buf);
}
