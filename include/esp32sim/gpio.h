#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace esp32sim {

enum class PinMode : uint8_t {
    INPUT = 0,
    OUTPUT = 1,
    INPUT_PULLUP = 2,
    INPUT_PULLDOWN = 3,
    OUTPUT_OPEN_DRAIN = 4,  // T2 may use; reserved
};

class PinRegistry {
public:
    static PinRegistry& instance();
    static constexpr int MAX_PIN = 63;  // covers ESP32-S3 (0..48) with margin

    using Listener = std::function<void(int old_level, int new_level)>;

    void reset();

    int get_level(int pin) const;
    void set_level(int pin, int level);  // emits GPIO_WRITE event

    PinMode get_mode(int pin) const;
    void set_mode(int pin, PinMode m);   // emits GPIO_PIN_MODE event

    // Listener fires synchronously when set_level changes the level. Used by
    // attachInterrupt and by test code (Sim::gpio(N).onChange(...)).
    void add_listener(int pin, Listener cb);

private:
    PinRegistry() { reset(); }
    struct PinState {
        int level = 0;
        PinMode mode = PinMode::INPUT;
        std::vector<Listener> listeners;
    };
    PinState pins_[MAX_PIN + 1];
    static bool valid(int pin) { return pin >= 0 && pin <= MAX_PIN; }
};

}  // namespace esp32sim
