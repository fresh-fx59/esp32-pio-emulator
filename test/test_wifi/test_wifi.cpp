#include <Arduino.h>
#include <WiFi.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) { esp32sim::Sim::reset(); }
void tearDown(void) {}

void test_wifi_starts_idle(void) {
    TEST_ASSERT_EQUAL_UINT8(WL_IDLE_STATUS, WiFi.status());
}

void test_wifi_begin_transitions_to_connected(void) {
    WiFi.begin("MyNetwork", "password123");
    TEST_ASSERT_EQUAL_UINT8(WL_CONNECTED, WiFi.status());
    TEST_ASSERT_EQUAL_STRING("MyNetwork", WiFi.SSID());
}

void test_wifi_disconnect(void) {
    WiFi.begin("MyNetwork", "pass");
    WiFi.disconnect();
    TEST_ASSERT_EQUAL_UINT8(WL_DISCONNECTED, WiFi.status());
}

void test_wifi_localIP_after_connect(void) {
    WiFi.begin("Net", "pass");
    auto ip = WiFi.localIP();
    TEST_ASSERT_EQUAL_STRING("192.168.1.42", ip.toString().c_str());
}

void test_wifi_RSSI_default(void) {
    TEST_ASSERT_EQUAL_INT(-50, WiFi.RSSI());
    esp32sim::Network::instance().set_rssi(-72);
    TEST_ASSERT_EQUAL_INT(-72, WiFi.RSSI());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_wifi_starts_idle);
    RUN_TEST(test_wifi_begin_transitions_to_connected);
    RUN_TEST(test_wifi_disconnect);
    RUN_TEST(test_wifi_localIP_after_connect);
    RUN_TEST(test_wifi_RSSI_default);
    return UNITY_END();
}
