#include <esp32sim_unity/esp32sim.h>

extern "C" void setup(void);
extern "C" void loop(void);

// Weak default implementations so tests that don't define a sketch (e.g.,
// pure unit tests of core abstractions) link cleanly. User sketches' strong
// definitions of setup() / loop() override these. The weak attribute is
// GCC/Clang-specific; PlatformIO native env on Linux/macOS uses GCC or
// Apple Clang, both of which support it.
extern "C" __attribute__((weak)) void setup(void) {}
extern "C" __attribute__((weak)) void loop(void) {}

namespace esp32sim {

void Sim::reset() {
    VirtualClock::instance().reset();
    EventLog::instance().reset();
    PinRegistry::instance().reset();
    UartChannel::reset_all();
    Network::instance().reset();
    Nvs::instance().reset();
    FileSystem::instance().reset();
    Sleep::instance().reset();
}

void Sim::runSetup() { setup(); }

void Sim::runLoop(int n) {
    for (int i = 0; i < n; ++i) loop();
}

bool Sim::runUntil(std::function<bool()> pred, uint64_t timeoutMs) {
    uint64_t deadline = VirtualClock::instance().now_ms() + timeoutMs;
    while (VirtualClock::instance().now_ms() < deadline) {
        loop();
        if (pred()) return true;
        VirtualClock::instance().advance_ms(1);
        if (pred()) return true;
    }
    return false;
}

void Sim::advanceMs(uint64_t ms) { VirtualClock::instance().advance_ms(ms); }
void Sim::advanceUs(uint64_t us) { VirtualClock::instance().advance_us(us); }
uint64_t Sim::nowMs() { return VirtualClock::instance().now_ms(); }
uint64_t Sim::nowUs() { return VirtualClock::instance().now_us(); }

// GpioRef
int Sim::GpioRef::level() const { return PinRegistry::instance().get_level(pin_); }
PinMode Sim::GpioRef::mode() const { return PinRegistry::instance().get_mode(pin_); }
void Sim::GpioRef::setLevel(int v) { PinRegistry::instance().set_level(pin_, v); }
void Sim::GpioRef::pulse(int v, uint64_t ms) {
    int prev = level();
    setLevel(v);
    VirtualClock::instance().advance_ms(ms);
    setLevel(prev);
}

// UartRef
std::string Sim::UartRef::drainTx() { return UartChannel::for_index(n_).drain_tx(); }
std::string Sim::UartRef::txAll() const { return UartChannel::for_index(n_).tx_history(); }
bool Sim::UartRef::txContains(const std::string& s) const {
    return UartChannel::for_index(n_).tx_history().find(s) != std::string::npos;
}
void Sim::UartRef::inject(const std::string& s) {
    UartChannel::for_index(n_).rx_inject(s);
}

// EventQuery
std::vector<Event> Sim::EventQuery::all() const {
    auto& log = EventLog::instance();
    return log.filter([this](const Event& e) {
        if (have_kind_ && e.kind != kind_) return false;
        if (have_pin_ && e.pin != pin_) return false;
        if (have_after_ && e.timestamp_us <= after_) return false;
        return true;
    });
}
size_t Sim::EventQuery::count() const { return all().size(); }

}  // namespace esp32sim
