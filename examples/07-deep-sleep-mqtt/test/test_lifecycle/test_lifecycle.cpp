#include <Arduino.h>
#include <Preferences.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) { esp32sim::Sim::reset(); }
void tearDown(void) {}

void test_first_boot_no_creds_starts_ble(void) {
    esp32sim::Sim::runSetup();
    TEST_ASSERT_TRUE(esp32sim::Ble::instance().initialized());
    TEST_ASSERT_TRUE(esp32sim::Ble::instance().advertising());
    // Boot count incremented to 1.
    Preferences p;
    p.begin("config");
    TEST_ASSERT_EQUAL_UINT32(1, p.getUInt("boots"));
}

void test_with_creds_connects_publishes_sleeps(void) {
    // Pre-seed NVS credentials (simulates a previous BLE provisioning).
    Preferences p;
    p.begin("config");
    p.putString("ssid", "Home");
    p.putString("pass", "secret");
    p.end();

    esp32sim::Sim::runSetup();

    TEST_ASSERT_EQUAL_INT(2 /*CONNECTED*/, (int)esp32sim::Network::instance().wifi_state());
    auto& publishes = esp32sim::Network::instance().mqtt_publishes();
    bool published = false;
    for (auto& p : publishes) {
        if (p.topic == "telemetry/sensor" && p.payload.find("boot") != std::string::npos) {
            published = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(published);

    // Deep sleep was requested for 60 seconds.
    TEST_ASSERT_EQUAL_INT(1, esp32sim::Sleep::instance().deep_sleep_count());
    TEST_ASSERT_EQUAL_UINT64(60000000ULL, esp32sim::Sleep::instance().last_sleep_us());
}

void test_boot_count_persists_across_runs(void) {
    Preferences p;
    p.begin("config");
    p.putString("ssid", "Home");
    p.putString("pass", "x");
    p.end();

    esp32sim::Sim::runSetup();
    Preferences q;
    q.begin("config");
    TEST_ASSERT_EQUAL_UINT32(1, q.getUInt("boots"));
    q.end();

    // "Wake" from deep sleep — sketch's setup runs again. NVS persists.
    esp32sim::Sim::runSetup();
    Preferences r;
    r.begin("config");
    TEST_ASSERT_EQUAL_UINT32(2, r.getUInt("boots"));
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_first_boot_no_creds_starts_ble);
    RUN_TEST(test_with_creds_connects_publishes_sleeps);
    RUN_TEST(test_boot_count_persists_across_runs);
    return UNITY_END();
}
