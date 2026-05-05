// include/esp32sim/strict.h — strict-mode contract checker (v1.1)
//
// When enabled, the simulator's existing fakes upgrade their silent failures
// to recorded "violations" against the chip / API contract. Rules are derived
// from authoritative sources (datasheets, ESP-IDF docs, Arduino-ESP32 docs)
// and each has a unique error code (ESP_SIM_EXXX) for grep-able remediation.
//
// Each violation records:
//   - code (e.g. "ESP_SIM_E001")
//   - human message
//   - virtual timestamp at which it fired
//
// Tests assert via Strict::instance().has("ESP_SIM_E001") or .count() / .all().
//
// Rules are AUTONOMOUS — they don't require user-authored assertions. Drop a
// sketch in, enable strict mode, and the chip's contract IS the oracle.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace esp32sim {

struct Violation {
    std::string code;        // "ESP_SIM_E001"
    std::string message;     // human-readable, includes context (pin, addr, etc.)
    uint64_t timestamp_us = 0;  // virtual time when triggered
};

class Strict {
public:
    static Strict& instance();
    void reset();

    void enable(bool e = true) { enabled_ = e; }
    bool enabled() const { return enabled_; }

    void violation(const std::string& code, const std::string& message);

    const std::vector<Violation>& all() const { return violations_; }
    bool any() const { return !violations_.empty(); }
    bool has(const std::string& code) const;
    size_t count() const { return violations_.size(); }
    size_t count(const std::string& code) const;

    // Pretty-print all violations to stdout (used by verify CLI / reports).
    void print_report() const;

private:
    Strict() = default;
    bool enabled_ = false;
    std::vector<Violation> violations_;
};

}  // namespace esp32sim
