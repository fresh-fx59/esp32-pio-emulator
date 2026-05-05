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

// Severity of a contract violation:
//   ERROR    — definitely wrong; will cause real-hardware misbehavior, silent
//              data loss, crash, or brick. Tests should fail on these.
//   WARNING  — fragile or suboptimal pattern; works at runtime but is worth
//              flagging. Tests typically surface these as info, not failure.
enum class Severity : uint8_t {
    ERROR = 0,
    WARNING = 1,
};

struct Violation {
    std::string code;        // "ESP_SIM_E001"
    std::string message;     // human-readable, includes context (pin, addr, etc.)
    Severity severity = Severity::ERROR;
    uint64_t timestamp_us = 0;  // virtual time when triggered
};

class Strict {
public:
    static Strict& instance();
    void reset();

    void enable(bool e = true) { enabled_ = e; }
    bool enabled() const { return enabled_; }

    // Default severity = ERROR for back-compat with v1.1 callers.
    void violation(const std::string& code, const std::string& message,
                   Severity severity = Severity::ERROR);

    const std::vector<Violation>& all() const { return violations_; }
    std::vector<Violation> errors() const;
    std::vector<Violation> warnings() const;

    bool any() const { return !violations_.empty(); }
    bool has_errors() const;
    bool has_warnings() const;

    bool has(const std::string& code) const;
    size_t count() const { return violations_.size(); }
    size_t count(const std::string& code) const;
    size_t error_count() const;
    size_t warning_count() const;

    // Pretty-print all violations to stdout (errors first, warnings second).
    void print_report() const;

private:
    Strict() = default;
    bool enabled_ = false;
    std::vector<Violation> violations_;
};

}  // namespace esp32sim
