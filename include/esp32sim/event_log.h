#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace esp32sim {

enum class EventKind : uint8_t {
    GPIO_WRITE = 1,
    GPIO_PIN_MODE,
    UART_TX,
    UART_RX,
    INTERRUPT_FIRED,
    // T2+ kinds (I2C_TXN, SPI_TXN, ADC_READ, PWM_WRITE, TIMER_FIRED) added later.
};

struct Event {
    // T1 keeps the payload narrow: one int pin + one int value covers
    // GPIO/UART/interrupt cases. T2 will widen to a std::variant when I2C
    // transactions need (addr, vector<uint8_t>) and similar.
    EventKind kind;
    int pin = 0;
    int value = 0;
    uint64_t timestamp_us = 0;  // populated by EventLog::emit
};

class EventLog {
public:
    static EventLog& instance();

    void emit(Event ev);  // timestamp_us populated from VirtualClock if 0
    void reset();

    size_t count() const { return events_.size(); }
    const std::vector<Event>& all() const { return events_; }

    std::vector<Event> filter(std::function<bool(const Event&)> pred) const;
    std::vector<Event> between(uint64_t start_us, uint64_t end_us) const;
    std::vector<Event> by_kind(EventKind k) const {
        return filter([k](const Event& e) { return e.kind == k; });
    }

private:
    EventLog() = default;
    std::vector<Event> events_;
};

}  // namespace esp32sim
