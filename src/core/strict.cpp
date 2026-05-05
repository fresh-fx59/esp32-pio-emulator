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

void Strict::violation(const std::string& code, const std::string& message,
                       Severity severity) {
    if (!enabled_) return;
    Violation v;
    v.code = code;
    v.message = message;
    v.severity = severity;
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

std::vector<Violation> Strict::errors() const {
    std::vector<Violation> out;
    for (const auto& v : violations_) {
        if (v.severity == Severity::ERROR) out.push_back(v);
    }
    return out;
}

std::vector<Violation> Strict::warnings() const {
    std::vector<Violation> out;
    for (const auto& v : violations_) {
        if (v.severity == Severity::WARNING) out.push_back(v);
    }
    return out;
}

bool Strict::has_errors() const {
    for (const auto& v : violations_) {
        if (v.severity == Severity::ERROR) return true;
    }
    return false;
}

bool Strict::has_warnings() const {
    for (const auto& v : violations_) {
        if (v.severity == Severity::WARNING) return true;
    }
    return false;
}

size_t Strict::error_count() const {
    size_t n = 0;
    for (const auto& v : violations_) {
        if (v.severity == Severity::ERROR) ++n;
    }
    return n;
}

size_t Strict::warning_count() const {
    size_t n = 0;
    for (const auto& v : violations_) {
        if (v.severity == Severity::WARNING) ++n;
    }
    return n;
}

void Strict::print_report() const {
    if (violations_.empty()) {
        std::printf("[strict-mode] no violations\n");
        return;
    }
    size_t e = error_count();
    size_t w = warning_count();
    std::printf("[strict-mode] %zu violation(s): %zu error(s), %zu warning(s)\n",
                violations_.size(), e, w);
    if (e > 0) {
        std::printf("  ERRORS (must-fix):\n");
        for (const auto& v : violations_) {
            if (v.severity != Severity::ERROR) continue;
            std::printf("    [%s @ t=%llu us] %s\n",
                        v.code.c_str(),
                        (unsigned long long)v.timestamp_us,
                        v.message.c_str());
        }
    }
    if (w > 0) {
        std::printf("  WARNINGS (recommendations):\n");
        for (const auto& v : violations_) {
            if (v.severity != Severity::WARNING) continue;
            std::printf("    [%s @ t=%llu us] %s\n",
                        v.code.c_str(),
                        (unsigned long long)v.timestamp_us,
                        v.message.c_str());
        }
    }
}

}  // namespace esp32sim
