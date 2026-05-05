#include <BLEDevice.h>
#include <esp32sim/ble.h>
#include <esp32sim/strict.h>

#include <memory>
#include <vector>

namespace {
// Persistent storage so returned pointers stay valid for the test lifetime.
std::vector<std::unique_ptr<BLEServer>>          servers;
std::vector<std::unique_ptr<BLEService>>         services;
std::vector<std::unique_ptr<BLECharacteristic>>  chars;
std::unique_ptr<BLEAdvertising>                  advertising;
}  // namespace

void BLEDevice::init(const std::string& device_name) {
    esp32sim::Ble::instance().mark_initialized(device_name);
}

BLEServer* BLEDevice::createServer() {
    if (esp32sim::Strict::instance().enabled() && !esp32sim::Ble::instance().initialized()) {
        esp32sim::Strict::instance().violation(
            "ESP_SIM_E080",
            "BLEDevice::createServer() called before BLEDevice::init()");
    }
    servers.push_back(std::make_unique<BLEServer>());
    return servers.back().get();
}

BLEAdvertising* BLEDevice::getAdvertising() {
    if (!advertising) advertising = std::make_unique<BLEAdvertising>();
    return advertising.get();
}

BLEService* BLEServer::createService(const std::string& uuid) {
    if (esp32sim::Strict::instance().enabled() && !esp32sim::Ble::instance().initialized()) {
        esp32sim::Strict::instance().violation(
            "ESP_SIM_E081",
            "BLEServer::createService() called before BLEDevice::init()");
    }
    esp32sim::Ble::instance().add_service(uuid);
    services.push_back(std::make_unique<BLEService>(uuid));
    return services.back().get();
}

BLECharacteristic* BLEService::createCharacteristic(const std::string& uuid, uint32_t properties) {
    chars.push_back(std::make_unique<BLECharacteristic>(uuid, properties));
    return chars.back().get();
}

void BLEAdvertising::start() { esp32sim::Ble::instance().start_advertising(); }
void BLEAdvertising::stop()  { esp32sim::Ble::instance().stop_advertising(); }
void BLEAdvertising::addServiceUUID(const std::string& /*uuid*/) {}
