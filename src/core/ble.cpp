#include <esp32sim/ble.h>

namespace esp32sim {

Ble& Ble::instance() {
    static Ble b;
    return b;
}

void Ble::reset() {
    initialized_ = false;
    advertising_ = false;
    device_name_.clear();
    service_uuids_.clear();
}

void Ble::mark_initialized(const std::string& name) {
    initialized_ = true;
    device_name_ = name;
}

}  // namespace esp32sim
