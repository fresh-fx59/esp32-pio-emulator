#include <esp32sim/clock.h>
#include <esp32sim/event_log.h>

namespace esp32sim {

EventLog& EventLog::instance() {
    static EventLog l;
    return l;
}

void EventLog::emit(Event ev) {
    if (ev.timestamp_us == 0) {
        ev.timestamp_us = VirtualClock::instance().now_us();
    }
    events_.push_back(ev);
}

void EventLog::reset() { events_.clear(); }

std::vector<Event> EventLog::filter(std::function<bool(const Event&)> pred) const {
    std::vector<Event> out;
    for (const auto& e : events_) {
        if (pred(e)) out.push_back(e);
    }
    return out;
}

std::vector<Event> EventLog::between(uint64_t start_us, uint64_t end_us) const {
    return filter([start_us, end_us](const Event& e) {
        return e.timestamp_us >= start_us && e.timestamp_us <= end_us;
    });
}

}  // namespace esp32sim
