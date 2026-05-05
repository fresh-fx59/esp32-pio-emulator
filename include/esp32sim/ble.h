// include/esp32sim/ble.h — BLE stub state (T4 alpha; ADR D7 = stub-level)
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace esp32sim {

class Ble {
public:
    static Ble& instance();
    void reset();

    // Test-side inspection: what the sketch advertised / configured.
    bool initialized() const { return initialized_; }
    std::string device_name() const { return device_name_; }
    bool advertising() const { return advertising_; }
    const std::vector<std::string>& service_uuids() const { return service_uuids_; }

    // Sketch-side recording (called from BLEDevice fake)
    void mark_initialized(const std::string& name);
    void start_advertising() { advertising_ = true; }
    void stop_advertising() { advertising_ = false; }
    void add_service(const std::string& uuid) { service_uuids_.push_back(uuid); }

private:
    Ble() = default;
    bool initialized_ = false;
    bool advertising_ = false;
    std::string device_name_;
    std::vector<std::string> service_uuids_;
};

}  // namespace esp32sim
