// Unity-side test API. Tests #include this and write against esp32sim::Sim::*.
#pragma once

#include <esp32sim/esp32sim.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace esp32sim {

class Sim {
public:
    // Lifecycle
    static void reset();
    static void runSetup();
    static void runLoop(int n = 1);

    // Returns true if predicate became true within timeoutMs of virtual time.
    // Steps: call loop(), advance 1 ms of virtual time, check predicate.
    static bool runUntil(std::function<bool()> pred, uint64_t timeoutMs);

    // Time
    static void advanceMs(uint64_t ms);
    static void advanceUs(uint64_t us);
    static uint64_t nowMs();
    static uint64_t nowUs();

    // GPIO
    class GpioRef {
    public:
        explicit GpioRef(int pin) : pin_(pin) {}
        int level() const;
        PinMode mode() const;
        void setLevel(int v);                 // simulate external driver
        void pulse(int level, uint64_t ms);   // for buttons / debouncing tests

    private:
        int pin_;
    };
    static GpioRef gpio(int pin) { return GpioRef(pin); }

    // UART
    class UartRef {
    public:
        explicit UartRef(int n) : n_(n) {}
        std::string drainTx();
        std::string txAll() const;
        bool txContains(const std::string& s) const;
        void inject(const std::string& s);

    private:
        int n_;
    };
    static UartRef uart(int n = 0) { return UartRef(n); }

    // Event query
    class EventQuery {
    public:
        EventQuery& kind(EventKind k) {
            kind_ = k;
            have_kind_ = true;
            return *this;
        }
        EventQuery& pin(int p) {
            pin_ = p;
            have_pin_ = true;
            return *this;
        }
        EventQuery& after(uint64_t timestamp_us) {
            after_ = timestamp_us;
            have_after_ = true;
            return *this;
        }
        size_t count() const;
        std::vector<Event> all() const;

    private:
        EventKind kind_ = EventKind::GPIO_WRITE;
        bool have_kind_ = false;
        int pin_ = 0;
        bool have_pin_ = false;
        uint64_t after_ = 0;
        bool have_after_ = false;
    };
    static EventQuery events() { return EventQuery(); }
};

}  // namespace esp32sim
