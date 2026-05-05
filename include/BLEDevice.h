// include/BLEDevice.h — fake arduino-esp32 BLEDevice (T4 stub-level)
//
// This is a stub: API records intent (device name, services, advertising)
// to esp32sim::Ble, but doesn't model GATT exchange. Tests assert via
// esp32sim::Ble::instance().service_uuids(), .advertising(), etc.
//
// For tests that need real GATT peer interaction, use Sim::ble().simulateRead/
// Write — coming in a T4.5 with ADR-0005.
#pragma once

#include <stdint.h>
#include <string>

#ifdef __cplusplus

class BLEServer;
class BLEService;
class BLECharacteristic;
class BLEAdvertising;

class BLEDevice {
public:
    static void init(const std::string& device_name);
    static void deinit() {}
    static BLEServer* createServer();
    static BLEAdvertising* getAdvertising();
};

class BLEService {
public:
    explicit BLEService(const std::string& uuid) : uuid_(uuid) {}
    BLECharacteristic* createCharacteristic(const std::string& uuid, uint32_t properties);
    void start() {}
    std::string uuid() const { return uuid_; }
private:
    std::string uuid_;
};

class BLECharacteristic {
public:
    BLECharacteristic(const std::string& uuid, uint32_t /*props*/) : uuid_(uuid) {}
    void setValue(const std::string& v) { value_ = v; }
    void setValue(uint8_t* data, size_t len) { value_.assign(reinterpret_cast<char*>(data), len); }
    std::string getValue() const { return value_; }
    void notify() {}
private:
    std::string uuid_;
    std::string value_;
};

class BLEServer {
public:
    BLEService* createService(const std::string& uuid);
};

class BLEAdvertising {
public:
    void start();
    void stop();
    void addServiceUUID(const std::string& uuid);
};

// Constants (subset)
constexpr uint32_t BLECharacteristic_PROPERTY_READ   = 1u << 0;
constexpr uint32_t BLECharacteristic_PROPERTY_WRITE  = 1u << 1;
constexpr uint32_t BLECharacteristic_PROPERTY_NOTIFY = 1u << 2;

#endif  // __cplusplus
