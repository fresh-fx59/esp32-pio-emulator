#include <esp32sim/clock.h>
#include <esp32sim/strict.h>

#include <cstdio>

namespace esp32sim {

Strict& Strict::instance() {
    static Strict s;
    return s;
}

void Strict::reset() {
    enabled_ = false;
    violations_.clear();
}

void Strict::violation(const std::string& code, const std::string& message) {
    if (!enabled_) return;
    Violation v;
    v.code = code;
    v.message = message;
    v.timestamp_us = VirtualClock::instance().now_us();
    violations_.push_back(std::move(v));
}

bool Strict::has(const std::string& code) const {
    for (const auto& v : violations_) {
        if (v.code == code) return true;
    }
    return false;
}

size_t Strict::count(const std::string& code) const {
    size_t n = 0;
    for (const auto& v : violations_) {
        if (v.code == code) ++n;
    }
    return n;
}

void Strict::print_report() const {
    if (violations_.empty()) {
        std::printf("[strict-mode] no violations\n");
        return;
    }
    std::printf("[strict-mode] %zu violation(s):\n", violations_.size());
    for (const auto& v : violations_) {
        std::printf("  [%s @ t=%llu us] %s\n",
                    v.code.c_str(),
                    (unsigned long long)v.timestamp_us,
                    v.message.c_str());
    }
}

}  // namespace esp32sim
