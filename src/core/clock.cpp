// src/core/clock.cpp
#include <esp32sim/clock.h>

namespace esp32sim {

VirtualClock& VirtualClock::instance() {
    static VirtualClock c;
    return c;
}

void VirtualClock::reset() {
    now_us_ = 0;
    next_handle_ = 1;
    schedule_.clear();
}

void VirtualClock::advance_us(uint64_t us) {
    uint64_t target = now_us_ + us;
    while (!schedule_.empty()) {
        auto it = schedule_.begin();
        if (it->first > target) break;
        now_us_ = it->first;
        auto cb = std::move(it->second.second);
        schedule_.erase(it);
        if (cb) cb();
    }
    now_us_ = target;
}

ScheduleHandle VirtualClock::schedule_at(uint64_t at_us, std::function<void()> cb) {
    if (at_us < now_us_) at_us = now_us_;  // past times fire on next advance
    ScheduleHandle h = next_handle_++;
    schedule_.emplace(at_us, std::make_pair(h, std::move(cb)));
    return h;
}

bool VirtualClock::cancel(ScheduleHandle h) {
    for (auto it = schedule_.begin(); it != schedule_.end(); ++it) {
        if (it->second.first == h) {
            schedule_.erase(it);
            return true;
        }
    }
    return false;
}

}  // namespace esp32sim
