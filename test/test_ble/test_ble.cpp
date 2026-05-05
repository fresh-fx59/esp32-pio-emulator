#include <Arduino.h>
#include <BLEDevice.h>
#include <esp32sim/ble.h>
#include <unity.h>

void setUp(void) { esp32sim::Ble::instance().reset(); }
void tearDown(void) {}

void test_BLEDevice_init_records_name(void) {
    BLEDevice::init("MyDevice");
    TEST_ASSERT_TRUE(esp32sim::Ble::instance().initialized());
    TEST_ASSERT_EQUAL_STRING("MyDevice", esp32sim::Ble::instance().device_name().c_str());
}

void test_createService_records_uuid(void) {
    BLEDevice::init("X");
    auto* server = BLEDevice::createServer();
    server->createService("00001234-0000-1000-8000-00805f9b34fb");
    auto& uuids = esp32sim::Ble::instance().service_uuids();
    TEST_ASSERT_EQUAL_size_t(1, uuids.size());
    TEST_ASSERT_EQUAL_STRING("00001234-0000-1000-8000-00805f9b34fb", uuids[0].c_str());
}

void test_advertising_state_recorded(void) {
    BLEDevice::init("X");
    auto* adv = BLEDevice::getAdvertising();
    TEST_ASSERT_FALSE(esp32sim::Ble::instance().advertising());
    adv->start();
    TEST_ASSERT_TRUE(esp32sim::Ble::instance().advertising());
    adv->stop();
    TEST_ASSERT_FALSE(esp32sim::Ble::instance().advertising());
}

void test_characteristic_set_get_value(void) {
    BLEDevice::init("X");
    auto* server = BLEDevice::createServer();
    auto* svc = server->createService("svc");
    auto* ch = svc->createCharacteristic(
        "char", BLECharacteristic_PROPERTY_READ | BLECharacteristic_PROPERTY_NOTIFY);
    ch->setValue("hello");
    TEST_ASSERT_EQUAL_STRING("hello", ch->getValue().c_str());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_BLEDevice_init_records_name);
    RUN_TEST(test_createService_records_uuid);
    RUN_TEST(test_advertising_state_recorded);
    RUN_TEST(test_characteristic_set_get_value);
    return UNITY_END();
}
