#include <esp32sim/sleep.h>

namespace esp32sim {

Sleep& Sleep::instance() {
    static Sleep s;
    return s;
}

void Sleep::reset() {
    next_wake_ = WakeCause::UNDEFINED;
    last_wake_ = WakeCause::UNDEFINED;
    last_sleep_us_ = 0;
    deep_sleep_count_ = 0;
}

void Sleep::record_deep_sleep(uint64_t us) {
    last_sleep_us_ = us;
    deep_sleep_count_++;
    last_wake_ = next_wake_;
}

}  // namespace esp32sim
