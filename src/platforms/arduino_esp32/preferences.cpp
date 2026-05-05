#include <Preferences.h>
#include <esp32sim/storage.h>

bool Preferences::clear() {
    if (!opened_) return false;
    esp32sim::Nvs::instance().clear_namespace(ns_);
    return true;
}

size_t Preferences::putString(const char* key, const char* value) {
    if (!opened_ || !key || !value) return 0;
    esp32sim::Nvs::instance().set_string(ns_, key, value);
    return std::string(value).size();
}
std::string Preferences::getString(const char* key, const std::string& def) {
    if (!opened_ || !key) return def;
    std::string out;
    return esp32sim::Nvs::instance().get_string(ns_, key, out) ? out : def;
}

size_t Preferences::putUInt(const char* key, uint32_t value) {
    if (!opened_ || !key) return 0;
    esp32sim::Nvs::instance().set_uint(ns_, key, value);
    return sizeof(uint32_t);
}
uint32_t Preferences::getUInt(const char* key, uint32_t def) {
    if (!opened_ || !key) return def;
    uint32_t out = 0;
    return esp32sim::Nvs::instance().get_uint(ns_, key, out) ? out : def;
}

size_t Preferences::putInt(const char* key, int32_t value) {
    if (!opened_ || !key) return 0;
    esp32sim::Nvs::instance().set_int(ns_, key, value);
    return sizeof(int32_t);
}
int32_t Preferences::getInt(const char* key, int32_t def) {
    if (!opened_ || !key) return def;
    int32_t out = 0;
    return esp32sim::Nvs::instance().get_int(ns_, key, out) ? out : def;
}

size_t Preferences::putBool(const char* key, bool value) {
    if (!opened_ || !key) return 0;
    esp32sim::Nvs::instance().set_bool(ns_, key, value);
    return 1;
}
bool Preferences::getBool(const char* key, bool def) {
    if (!opened_ || !key) return def;
    bool out = false;
    return esp32sim::Nvs::instance().get_bool(ns_, key, out) ? out : def;
}

bool Preferences::remove(const char* key) {
    if (!opened_ || !key) return false;
    return esp32sim::Nvs::instance().remove(ns_, key);
}

bool Preferences::isKey(const char* key) {
    if (!opened_ || !key) return false;
    return esp32sim::Nvs::instance().has(ns_, key);
}
