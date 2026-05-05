#include <esp32_hwtimer.h>
#include <esp32sim/clock.h>

#include <vector>

namespace {

struct TimerState {
    uint16_t divider = 80;       // default = 1us tick on ESP32 80MHz APB clock
    bool count_up = true;
    void (*isr)(void) = nullptr;
    uint64_t alarm_value = 0;    // in ticks (per divider)
    bool autoreload = false;
    bool enabled = false;
    esp32sim::ScheduleHandle handle = 0;
    uint64_t started_at_us = 0;
};

// Fixed-size pool keyed by num (real ESP32 has 4 hw timers).
constexpr int kMaxTimers = 4;
TimerState pool[kMaxTimers];

uint64_t ticks_to_us(uint64_t ticks, uint16_t divider) {
    // ESP32 base clock: 80 MHz. Tick period = divider / 80MHz seconds = divider/80 us.
    // ticks * divider / 80 = us
    return (ticks * divider) / 80;
}

void schedule_alarm(int idx) {
    auto& t = pool[idx];
    if (!t.enabled) return;
    uint64_t at = esp32sim::VirtualClock::instance().now_us() +
                  ticks_to_us(t.alarm_value, t.divider);
    t.handle = esp32sim::VirtualClock::instance().schedule_at(at, [idx]() {
        auto& t2 = pool[idx];
        if (t2.enabled && t2.isr) t2.isr();
        if (t2.enabled && t2.autoreload) {
            schedule_alarm(idx);
        } else if (t2.enabled) {
            t2.enabled = false;
        }
    });
}

}  // namespace

extern "C" {

hw_timer_t* timerBegin(uint8_t num, uint16_t divider, bool count_up) {
    if (num >= kMaxTimers) return nullptr;
    auto& t = pool[num];
    t = TimerState{};
    t.divider = divider ? divider : 80;
    t.count_up = count_up;
    t.started_at_us = esp32sim::VirtualClock::instance().now_us();
    return reinterpret_cast<hw_timer_t*>(&pool[num]);
}

void timerEnd(hw_timer_t* timer) {
    if (!timer) return;
    auto* t = reinterpret_cast<TimerState*>(timer);
    if (t->handle) esp32sim::VirtualClock::instance().cancel(t->handle);
    *t = TimerState{};
}

void timerAttachInterrupt(hw_timer_t* timer, void (*fn)(void), bool /*edge*/) {
    if (!timer) return;
    reinterpret_cast<TimerState*>(timer)->isr = fn;
}

void timerDetachInterrupt(hw_timer_t* timer) {
    if (!timer) return;
    reinterpret_cast<TimerState*>(timer)->isr = nullptr;
}

void timerAlarmWrite(hw_timer_t* timer, uint64_t alarm_value, bool autoreload) {
    if (!timer) return;
    auto* t = reinterpret_cast<TimerState*>(timer);
    t->alarm_value = alarm_value;
    t->autoreload = autoreload;
}

void timerAlarmEnable(hw_timer_t* timer) {
    if (!timer) return;
    auto* t = reinterpret_cast<TimerState*>(timer);
    if (t->enabled) return;  // already running
    t->enabled = true;
    int idx = (int)(t - pool);
    schedule_alarm(idx);
}

void timerAlarmDisable(hw_timer_t* timer) {
    if (!timer) return;
    auto* t = reinterpret_cast<TimerState*>(timer);
    t->enabled = false;
    if (t->handle) {
        esp32sim::VirtualClock::instance().cancel(t->handle);
        t->handle = 0;
    }
}

uint64_t timerRead(hw_timer_t* timer) {
    if (!timer) return 0;
    auto* t = reinterpret_cast<TimerState*>(timer);
    uint64_t elapsed_us = esp32sim::VirtualClock::instance().now_us() - t->started_at_us;
    // ticks = elapsed_us * 80 / divider
    return elapsed_us * 80 / t->divider;
}

}  // extern "C"
