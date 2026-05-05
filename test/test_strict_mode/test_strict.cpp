// test/test_strict_mode/test_strict.cpp
//
// One test per rule — verifies that the contract violation is recorded
// when strict mode is enabled. Each rule has a unique ESP_SIM_EXXX code.
#include <Arduino.h>
#include <BLEDevice.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) {
    esp32sim::Sim::reset();
    esp32sim::Strict::instance().enable();
}
void tearDown(void) {}

// ============================================================
// GPIO rules
// ============================================================

void test_E001_digitalWrite_without_pinMode(void) {
    digitalWrite(2, HIGH);
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E001"));
}

void test_E001_not_fired_when_pinMode_correct(void) {
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    TEST_ASSERT_FALSE(esp32sim::Strict::instance().has("ESP_SIM_E001"));
}

void test_E002_using_flash_pin(void) {
    // GPIO 28 is flash on ESP32-S3
    pinMode(28, OUTPUT);
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E002"));
}

void test_E002_not_fired_for_safe_pin(void) {
    pinMode(2, OUTPUT);
    TEST_ASSERT_FALSE(esp32sim::Strict::instance().has("ESP_SIM_E002"));
}

void test_E003_pin_out_of_range(void) {
    pinMode(99, OUTPUT);  // ESP32-S3 max is 48
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E003"));
}

void test_E004_analogRead_on_non_ADC_pin(void) {
    // GPIO 25 is not in ADC1 (1-10) or ADC2 (11-20) on ESP32-S3
    analogRead(25);
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E004"));
}

void test_E004_not_fired_for_ADC_capable_pin(void) {
    analogRead(5);  // ADC1
    TEST_ASSERT_FALSE(esp32sim::Strict::instance().has("ESP_SIM_E004"));
}

void test_E006_strapping_pin_warning(void) {
    pinMode(0, OUTPUT);  // GPIO 0 is strapping
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E006"));
}

// ============================================================
// Serial rules
// ============================================================

void test_E010_Serial_print_before_begin(void) {
    Serial.print("hello");
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E010"));
}

void test_E010_not_fired_after_begin(void) {
    Serial.begin(115200);
    Serial.print("hello");
    TEST_ASSERT_FALSE(esp32sim::Strict::instance().has("ESP_SIM_E010"));
}

// ============================================================
// I2C / Wire rules
// ============================================================

void test_E020_Wire_write_outside_transmission(void) {
    Wire.write((uint8_t)0xAA);  // no beginTransmission first
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E020"));
}

void test_E021_I2C_address_too_high(void) {
    Wire.beginTransmission((uint8_t)0x99);  // > 0x7F
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E021"));
}

void test_E022_double_beginTransmission(void) {
    Wire.beginTransmission((uint8_t)0x42);
    Wire.beginTransmission((uint8_t)0x43);  // didn't end the first
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E022"));
}

void test_E020_E022_not_fired_for_correct_idiom(void) {
    Wire.beginTransmission((uint8_t)0x42);
    Wire.write((uint8_t)0x01);
    Wire.endTransmission();
    Wire.beginTransmission((uint8_t)0x43);
    Wire.write((uint8_t)0x01);
    Wire.endTransmission();
    TEST_ASSERT_FALSE(esp32sim::Strict::instance().has("ESP_SIM_E020"));
    TEST_ASSERT_FALSE(esp32sim::Strict::instance().has("ESP_SIM_E022"));
}

// ============================================================
// Time rules
// ============================================================

void test_E040_delayMicroseconds_too_large(void) {
    delayMicroseconds(20000);
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E040"));
}

void test_E040_not_fired_for_safe_value(void) {
    delayMicroseconds(1000);
    TEST_ASSERT_FALSE(esp32sim::Strict::instance().has("ESP_SIM_E040"));
}

// ============================================================
// WiFi / network rules
// ============================================================

void test_E050_localIP_before_WiFi_begin(void) {
    WiFi.localIP();
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E050"));
}

void test_E050_RSSI_before_WiFi_begin(void) {
    WiFi.RSSI();
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E050"));
}

void test_E050_not_fired_after_begin(void) {
    WiFi.begin("net", "pass");
    WiFi.localIP();
    WiFi.RSSI();
    TEST_ASSERT_FALSE(esp32sim::Strict::instance().has("ESP_SIM_E050"));
}

void test_E051_HTTP_GET_without_begin(void) {
    HTTPClient http;
    http.GET();  // no http.begin(url) first
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E051"));
}

void test_E052_MQTT_publish_disconnected(void) {
    PubSubClient mqtt;
    mqtt.publish("topic", "payload");  // not connected
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E052"));
}

// ============================================================
// Preferences rules
// ============================================================

void test_E060_putString_without_begin(void) {
    Preferences p;
    p.putString("k", "v");  // no begin()
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E060"));
}

void test_E061_namespace_too_long(void) {
    Preferences p;
    p.begin("this_namespace_is_too_long");  // > 15 chars
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E061"));
}

void test_E061_not_fired_for_short_namespace(void) {
    Preferences p;
    p.begin("config");
    TEST_ASSERT_FALSE(esp32sim::Strict::instance().has("ESP_SIM_E061"));
}

// ============================================================
// PWM / LEDC rules
// ============================================================

void test_E070_ledcWrite_without_setup(void) {
    ledcWrite(3, 128);  // never called ledcSetup
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E070"));
}

void test_E071_LEDC_channel_out_of_range(void) {
    ledcSetup(15, 1000, 8);  // ESP32-S3 only has channels 0..7
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E071"));
}

// ============================================================
// BLE rules
// ============================================================

void test_E080_createServer_before_init(void) {
    BLEDevice::createServer();  // no init first
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E080"));
}

void test_E081_createService_before_init(void) {
    BLEDevice::init("X");
    auto* server = BLEDevice::createServer();
    esp32sim::Ble::instance().reset();  // simulate "uninit" between
    server->createService("uuid");
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E081"));
}

// ============================================================
// Strict mode infrastructure
// ============================================================

void test_strict_mode_off_records_nothing(void) {
    esp32sim::Strict::instance().enable(false);
    digitalWrite(2, HIGH);          // would be E001
    Wire.beginTransmission(0x99);   // would be E021
    delayMicroseconds(20000);       // would be E040
    TEST_ASSERT_EQUAL_size_t(0, esp32sim::Strict::instance().count());
}

void test_violations_carry_timestamp(void) {
    digitalWrite(2, HIGH);  // E001 at t=0
    esp32sim::Sim::advanceMs(500);
    Wire.beginTransmission(0x99);  // E021 at t=500ms
    auto& vs = esp32sim::Strict::instance().all();
    TEST_ASSERT_EQUAL_size_t(2, vs.size());
    TEST_ASSERT_EQUAL_UINT64(0, vs[0].timestamp_us);
    TEST_ASSERT_EQUAL_UINT64(500000, vs[1].timestamp_us);
}

void test_count_by_code(void) {
    digitalWrite(2, HIGH);
    digitalWrite(3, LOW);
    TEST_ASSERT_EQUAL_size_t(2, esp32sim::Strict::instance().count("ESP_SIM_E001"));
}

// ============================================================
// Severity classification (v1.2)
// ============================================================

void test_E001_is_classified_ERROR(void) {
    digitalWrite(2, HIGH);
    auto errors = esp32sim::Strict::instance().errors();
    TEST_ASSERT_GREATER_OR_EQUAL_size_t(1, errors.size());
    bool found_error = false;
    for (auto& v : errors) {
        if (v.code == "ESP_SIM_E001") {
            TEST_ASSERT_EQUAL_INT((int)esp32sim::Severity::ERROR, (int)v.severity);
            found_error = true;
        }
    }
    TEST_ASSERT_TRUE(found_error);
}

void test_E006_strapping_pin_is_WARNING(void) {
    pinMode(0, OUTPUT);  // strapping pin
    auto& strict = esp32sim::Strict::instance();
    TEST_ASSERT_TRUE(strict.has_warnings());
    auto warnings = strict.warnings();
    bool found = false;
    for (auto& w : warnings) {
        if (w.code == "ESP_SIM_E006") {
            TEST_ASSERT_EQUAL_INT((int)esp32sim::Severity::WARNING, (int)w.severity);
            found = true;
        }
    }
    TEST_ASSERT_TRUE(found);
}

void test_E007_USB_JTAG_pin_GPIO19(void) {
    pinMode(19, OUTPUT);
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E007"));
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has_warnings());
}

void test_E007_USB_JTAG_pin_GPIO20(void) {
    pinMode(20, OUTPUT);
    TEST_ASSERT_TRUE(esp32sim::Strict::instance().has("ESP_SIM_E007"));
}

void test_errors_and_warnings_separated(void) {
    digitalWrite(2, HIGH);          // ERROR (E001)
    Wire.beginTransmission(0x99);   // ERROR (E021)
    pinMode(19, INPUT);             // WARNING (E007 USB-JTAG)
    auto& strict = esp32sim::Strict::instance();
    TEST_ASSERT_TRUE(strict.has_errors());
    TEST_ASSERT_TRUE(strict.has_warnings());
    TEST_ASSERT_GREATER_OR_EQUAL_size_t(2, strict.error_count());
    TEST_ASSERT_GREATER_OR_EQUAL_size_t(1, strict.warning_count());
}

void test_clean_sketch_no_errors_or_warnings(void) {
    pinMode(5, OUTPUT);
    digitalWrite(5, HIGH);
    Serial.begin(115200);
    Serial.println("ok");
    auto& strict = esp32sim::Strict::instance();
    TEST_ASSERT_FALSE(strict.has_errors());
    TEST_ASSERT_FALSE(strict.has_warnings());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    // GPIO
    RUN_TEST(test_E001_digitalWrite_without_pinMode);
    RUN_TEST(test_E001_not_fired_when_pinMode_correct);
    RUN_TEST(test_E002_using_flash_pin);
    RUN_TEST(test_E002_not_fired_for_safe_pin);
    RUN_TEST(test_E003_pin_out_of_range);
    RUN_TEST(test_E004_analogRead_on_non_ADC_pin);
    RUN_TEST(test_E004_not_fired_for_ADC_capable_pin);
    RUN_TEST(test_E006_strapping_pin_warning);
    // Serial
    RUN_TEST(test_E010_Serial_print_before_begin);
    RUN_TEST(test_E010_not_fired_after_begin);
    // I2C
    RUN_TEST(test_E020_Wire_write_outside_transmission);
    RUN_TEST(test_E021_I2C_address_too_high);
    RUN_TEST(test_E022_double_beginTransmission);
    RUN_TEST(test_E020_E022_not_fired_for_correct_idiom);
    // Time
    RUN_TEST(test_E040_delayMicroseconds_too_large);
    RUN_TEST(test_E040_not_fired_for_safe_value);
    // WiFi/network
    RUN_TEST(test_E050_localIP_before_WiFi_begin);
    RUN_TEST(test_E050_RSSI_before_WiFi_begin);
    RUN_TEST(test_E050_not_fired_after_begin);
    RUN_TEST(test_E051_HTTP_GET_without_begin);
    RUN_TEST(test_E052_MQTT_publish_disconnected);
    // Preferences
    RUN_TEST(test_E060_putString_without_begin);
    RUN_TEST(test_E061_namespace_too_long);
    RUN_TEST(test_E061_not_fired_for_short_namespace);
    // PWM
    RUN_TEST(test_E070_ledcWrite_without_setup);
    RUN_TEST(test_E071_LEDC_channel_out_of_range);
    // BLE
    RUN_TEST(test_E080_createServer_before_init);
    RUN_TEST(test_E081_createService_before_init);
    // Infrastructure
    RUN_TEST(test_strict_mode_off_records_nothing);
    RUN_TEST(test_violations_carry_timestamp);
    RUN_TEST(test_count_by_code);
    // Severity classification (v1.2)
    RUN_TEST(test_E001_is_classified_ERROR);
    RUN_TEST(test_E006_strapping_pin_is_WARNING);
    RUN_TEST(test_E007_USB_JTAG_pin_GPIO19);
    RUN_TEST(test_E007_USB_JTAG_pin_GPIO20);
    RUN_TEST(test_errors_and_warnings_separated);
    RUN_TEST(test_clean_sketch_no_errors_or_warnings);
    return UNITY_END();
}
