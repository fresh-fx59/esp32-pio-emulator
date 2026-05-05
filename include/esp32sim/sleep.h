// include/esp32sim/sleep.h — deep-sleep + wake-reason simulation (T4)
#pragma once

#include <cstdint>

namespace esp32sim {

// Mirrors arduino-esp32's esp_sleep_wakeup_cause_t
enum class WakeCause : uint8_t {
    UNDEFINED = 0,
    EXT0 = 2,
    EXT1 = 3,
    TIMER = 4,
    TOUCHPAD = 5,
    ULP = 6,
};

class Sleep {
public:
    static Sleep& instance();
    void reset();

    // Test-side: arrange the next wake cause
    void set_next_wake_cause(WakeCause c) { next_wake_ = c; }
    WakeCause last_wake() const { return last_wake_; }

    // Sketch-side: deep_sleep_start records the request and "wakes" by
    // re-running setup with the test-set wake cause. The sim DOES NOT
    // actually loop the sketch; tests drive that explicitly.
    void record_deep_sleep(uint64_t us);
    uint64_t last_sleep_us() const { return last_sleep_us_; }
    int deep_sleep_count() const { return deep_sleep_count_; }

private:
    Sleep() = default;
    WakeCause next_wake_ = WakeCause::UNDEFINED;
    WakeCause last_wake_ = WakeCause::UNDEFINED;
    uint64_t last_sleep_us_ = 0;
    int deep_sleep_count_ = 0;
};

}  // namespace esp32sim
