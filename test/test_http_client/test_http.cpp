#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) { esp32sim::Sim::reset(); }
void tearDown(void) {}

void test_http_get_returns_seeded_response(void) {
    WiFi.begin("Net", "pass");
    esp32sim::HttpResponse seeded;
    seeded.code = 200;
    seeded.body = "{\"temp\":22.5}";
    esp32sim::Network::instance().seed_http_response("https://api.example.com/temp", seeded);

    HTTPClient http;
    http.begin("https://api.example.com/temp");
    int code = http.GET();
    TEST_ASSERT_EQUAL_INT(200, code);
    TEST_ASSERT_EQUAL_STRING("{\"temp\":22.5}", http.getString().c_str());
}

void test_http_get_unseeded_returns_404(void) {
    HTTPClient http;
    http.begin("https://nope.example.com/x");
    TEST_ASSERT_EQUAL_INT(404, http.GET());
}

void test_http_post_records_request(void) {
    HTTPClient http;
    http.begin("https://api.example.com/post");
    http.POST(std::string("{\"value\":42}"));

    auto& publishes = esp32sim::Network::instance().mqtt_publishes();
    bool found = false;
    for (auto& p : publishes) {
        if (p.topic == "HTTP:POST https://api.example.com/post" &&
            p.payload == "{\"value\":42}") {
            found = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(found);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_http_get_returns_seeded_response);
    RUN_TEST(test_http_get_unseeded_returns_404);
    RUN_TEST(test_http_post_records_request);
    return UNITY_END();
}
