// include/esp32sim/clock.h
#pragma once

#include <cstdint>
#include <functional>
#include <map>

namespace esp32sim {

using ScheduleHandle = uint64_t;

class VirtualClock {
public:
    static VirtualClock& instance();

    // Singletons aren't normally great, but the Arduino API (millis(), delay())
    // is implicitly singleton — there's exactly one notion of "now" in a sketch.
    // Resetting between tests is mandatory; setUp() must call reset().
    void reset();

    uint64_t now_us() const { return now_us_; }
    uint64_t now_ms() const { return now_us_ / 1000; }

    // Advance time, firing scheduled callbacks at their target timestamp.
    // Callbacks fire in timestamp order; if multiple at the same timestamp,
    // insertion order.
    void advance_us(uint64_t us);
    void advance_ms(uint64_t ms) { advance_us(ms * 1000); }

    // Schedule a callback to fire when now_us() reaches `at_us`. Returns a
    // handle the caller can use to cancel. Cancelled callbacks never fire.
    ScheduleHandle schedule_at(uint64_t at_us, std::function<void()> cb);
    bool cancel(ScheduleHandle h);

private:
    VirtualClock() = default;
    uint64_t now_us_ = 0;
    ScheduleHandle next_handle_ = 1;
    // Multimap so multiple callbacks at the same timestamp are preserved in
    // insertion order (multimap maintains insertion order for equal keys).
    std::multimap<uint64_t, std::pair<ScheduleHandle, std::function<void()>>> schedule_;
};

}  // namespace esp32sim
