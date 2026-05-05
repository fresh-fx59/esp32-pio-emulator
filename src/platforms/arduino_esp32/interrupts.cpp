#include <Arduino.h>
#include <esp32sim/gpio.h>

#include <array>

namespace {

struct IsrSlot {
    voidFuncPtr fn = nullptr;
    int mode = 0;
};

constexpr int kMaxPin = 64;
std::array<IsrSlot, kMaxPin> isrs{};

bool should_fire(int mode, int old_lvl, int new_lvl) {
    if (mode == RISING)  return old_lvl == 0 && new_lvl == 1;
    if (mode == FALLING) return old_lvl == 1 && new_lvl == 0;
    if (mode == CHANGE)  return old_lvl != new_lvl;
    if (mode == ONHIGH)  return new_lvl == 1 && old_lvl == 0;
    if (mode == ONLOW)   return new_lvl == 0 && old_lvl == 1;
    return false;
}

}  // namespace

extern "C" {

void attachInterrupt(uint8_t pin, voidFuncPtr isr, int mode) {
    if (pin >= kMaxPin) return;
    isrs[pin] = {isr, mode};
    // Listener captures by reference into isrs[]; if the slot is reassigned
    // the old listener becomes a no-op via the runtime nullptr check.
    esp32sim::PinRegistry::instance().add_listener(
        (int)pin, [pin](int old_lvl, int new_lvl) {
            const auto& s = isrs[pin];
            if (s.fn && should_fire(s.mode, old_lvl, new_lvl)) s.fn();
        });
}

void detachInterrupt(uint8_t pin) {
    if (pin >= kMaxPin) return;
    isrs[pin].fn = nullptr;
    isrs[pin].mode = 0;
    // Note: the underlying PinRegistry listener stays attached, but it
    // becomes a no-op because s.fn is now nullptr. PinRegistry doesn't
    // currently support listener removal; that's a known T2+ ergonomics
    // improvement.
}

}  // extern "C"
