#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

namespace {
std::string last_topic;
std::string last_payload;
}

void mqtt_callback(char* topic, uint8_t* payload, unsigned int length) {
    last_topic = topic;
    last_payload = std::string(reinterpret_cast<char*>(payload), length);
}

void setUp(void) {
    esp32sim::Sim::reset();
    last_topic.clear();
    last_payload.clear();
}
void tearDown(void) {}

void test_mqtt_connect_and_publish(void) {
    PubSubClient mqtt;
    mqtt.setServer("broker.local", 1883);
    TEST_ASSERT_TRUE(mqtt.connect("client-1"));
    TEST_ASSERT_TRUE(mqtt.publish("sensors/temp", "22.5"));

    auto& publishes = esp32sim::Network::instance().mqtt_publishes();
    TEST_ASSERT_EQUAL_size_t(1, publishes.size());
    TEST_ASSERT_EQUAL_STRING("sensors/temp", publishes[0].topic.c_str());
    TEST_ASSERT_EQUAL_STRING("22.5", publishes[0].payload.c_str());
}

void test_mqtt_subscribe_records(void) {
    PubSubClient mqtt;
    mqtt.connect("client-1");
    mqtt.subscribe("control/+");

    auto& subs = esp32sim::Network::instance().mqtt_subscribes();
    TEST_ASSERT_EQUAL_size_t(1, subs.size());
    TEST_ASSERT_EQUAL_STRING("control/+", subs[0].c_str());
}

void test_mqtt_loop_delivers_pending_message(void) {
    PubSubClient mqtt(mqtt_callback);
    mqtt.connect("c");
    mqtt.subscribe("control/led");
    esp32sim::Network::instance().mqtt_deliver("control/led", "ON");

    mqtt.loop();
    TEST_ASSERT_EQUAL_STRING("control/led", last_topic.c_str());
    TEST_ASSERT_EQUAL_STRING("ON", last_payload.c_str());
}

void test_mqtt_publish_when_disconnected_fails(void) {
    PubSubClient mqtt;
    TEST_ASSERT_FALSE(mqtt.publish("topic", "payload"));
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_mqtt_connect_and_publish);
    RUN_TEST(test_mqtt_subscribe_records);
    RUN_TEST(test_mqtt_loop_delivers_pending_message);
    RUN_TEST(test_mqtt_publish_when_disconnected_fails);
    return UNITY_END();
}
