#include <esp32sim/event_log.h>
#include <esp32sim/gpio.h>

namespace esp32sim {

PinRegistry& PinRegistry::instance() {
    static PinRegistry r;
    return r;
}

void PinRegistry::reset() {
    for (auto& p : pins_) {
        p.level = 0;
        p.mode = PinMode::INPUT;
        p.listeners.clear();
    }
}

int PinRegistry::get_level(int pin) const {
    if (!valid(pin)) return 0;
    const auto& p = pins_[pin];
    // INPUT_PULLUP/DOWN: if no driver has set a level, return the pull state.
    // For T1 simplicity we don't track "is anyone driving this pin" — so a
    // pullup pin reads high until set_level overrides it explicitly.
    if (p.level == 0 && p.mode == PinMode::INPUT_PULLUP) return 1;
    return p.level;
}

void PinRegistry::set_level(int pin, int level) {
    if (!valid(pin)) return;
    auto& p = pins_[pin];
    int old = p.level;
    p.level = level ? 1 : 0;
    EventLog::instance().emit(Event{EventKind::GPIO_WRITE, pin, p.level});
    if (old != p.level) {
        for (auto& l : p.listeners) l(old, p.level);
    }
}

PinMode PinRegistry::get_mode(int pin) const {
    if (!valid(pin)) return PinMode::INPUT;
    return pins_[pin].mode;
}

void PinRegistry::set_mode(int pin, PinMode m) {
    if (!valid(pin)) return;
    pins_[pin].mode = m;
    EventLog::instance().emit(Event{EventKind::GPIO_PIN_MODE, pin, (int)m});
}

void PinRegistry::add_listener(int pin, Listener cb) {
    if (!valid(pin)) return;
    pins_[pin].listeners.push_back(std::move(cb));
}

}  // namespace esp32sim
