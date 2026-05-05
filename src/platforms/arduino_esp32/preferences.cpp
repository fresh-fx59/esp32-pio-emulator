#include <Preferences.h>
#include <esp32sim/storage.h>
#include <esp32sim/strict.h>

#include <cstdio>

bool Preferences::begin(const char* ns, bool /*read_only*/) {
    auto& strict = esp32sim::Strict::instance();
    if (strict.enabled() && ns) {
        // NVS namespace limit is 15 chars + null per the IDF nvs.h API contract.
        size_t len = 0;
        while (ns[len] && len < 32) ++len;
        if (len > 15) {
            char buf[160];
            std::snprintf(buf, sizeof(buf),
                "Preferences.begin('%s') — NVS namespace must be ≤15 characters "
                "(this one is %zu)",
                ns, len);
            strict.violation("ESP_SIM_E061", buf);
        }
    }
    ns_ = ns ? ns : "";
    opened_ = true;
    return true;
}

bool Preferences::clear() {
    if (esp32sim::Strict::instance().enabled() && !opened_) {
        esp32sim::Strict::instance().violation(
            "ESP_SIM_E060",
            "Preferences.clear() called without prior Preferences.begin()");
    }
    if (!opened_) return false;
    esp32sim::Nvs::instance().clear_namespace(ns_);
    return true;
}

size_t Preferences::putString(const char* key, const char* value) {
    if (esp32sim::Strict::instance().enabled() && !opened_) {
        esp32sim::Strict::instance().violation(
            "ESP_SIM_E060",
            "Preferences.putString called without prior Preferences.begin()");
    }
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
