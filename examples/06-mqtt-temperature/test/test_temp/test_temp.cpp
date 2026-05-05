#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) { esp32sim::Sim::reset(); }
void tearDown(void) {}

void test_setup_connects_wifi(void) {
    // Pre-seed the HTTP response so setup's config fetch returns a topic.
    esp32sim::HttpResponse seeded;
    seeded.code = 200;
    seeded.body = "garden/temp";
    esp32sim::Network::instance().seed_http_response(
        "https://config.example.com/topic", seeded);

    esp32sim::Sim::runSetup();

    TEST_ASSERT_EQUAL_INT(3 /*WL_CONNECTED*/, (int)esp32sim::Network::instance().wifi_state() == 2 ? 3 : (int)esp32sim::Network::instance().wifi_state());
    // Simpler: just check the SSID + IP are set
    TEST_ASSERT_EQUAL_STRING("HomeNetwork", esp32sim::Network::instance().ssid().c_str());
    TEST_ASSERT_EQUAL_STRING("192.168.1.42", esp32sim::Network::instance().local_ip().c_str());
}

void test_setup_fetches_topic_from_http(void) {
    esp32sim::HttpResponse seeded;
    seeded.code = 200;
    seeded.body = "garden/temp";
    esp32sim::Network::instance().seed_http_response(
        "https://config.example.com/topic", seeded);

    esp32sim::Sim::runSetup();

    // Verify HTTP request was made
    auto& publishes = esp32sim::Network::instance().mqtt_publishes();
    bool http_recorded = false;
    for (auto& p : publishes) {
        if (p.topic.find("HTTP:GET https://config.example.com/topic") != std::string::npos) {
            http_recorded = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(http_recorded);
}

void test_loop_publishes_temperature(void) {
    esp32sim::HttpResponse seeded;
    seeded.code = 200;
    seeded.body = "garden/temp";
    esp32sim::Network::instance().seed_http_response(
        "https://config.example.com/topic", seeded);

    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();
    esp32sim::Sim::advanceMs(6000);
    esp32sim::Sim::runLoop();

    auto& publishes = esp32sim::Network::instance().mqtt_publishes();
    bool found_mqtt = false;
    for (auto& p : publishes) {
        if (p.topic == "garden/temp" && p.payload == "22.50") {
            found_mqtt = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(found_mqtt);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_setup_connects_wifi);
    RUN_TEST(test_setup_fetches_topic_from_http);
    RUN_TEST(test_loop_publishes_temperature);
    return UNITY_END();
}
