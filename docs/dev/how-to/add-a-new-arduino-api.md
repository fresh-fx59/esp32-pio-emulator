# How to add a new Arduino API

Adding support for a new arduino-esp32 function (say, `tone(pin, freq, duration)`) follows a
consistent pattern. This guide walks through it.

## The five-file pattern

Every new public API touches five places (most of them tests):

1. **Public declaration** — `include/Arduino.h` (or another public header). What consumers
   will `#include`.
2. **Internal core support** — usually `include/esp32sim/<area>.h` + `src/core/<area>.cpp`.
   The framework-neutral primitive(s) the new API forwards into.
3. **Adapter implementation** — `src/platforms/arduino_esp32/<file>.cpp`. The thin forwarder
   that translates Arduino API calls into core primitives.
4. **Unit tests for the core piece** — `test/test_core_<area>/`.
5. **Integration test for the Arduino-side wrapper** — usually a new test in
   `test/test_arduino_basics/` or a dedicated `test/test_<api>/`.

## Pattern in detail (taking a hypothetical `tone()` example)

### Step 1: Decide where the behavior lives in core

`tone(pin, frequency, duration_ms)` produces a square wave on a pin. In our model, this
maps to scheduling alternating `set_level` calls in `VirtualClock`. So no new core file is
needed — `core/clock.h` + `core/pin_registry.h` already provide the primitives.

### Step 2: Declare in `Arduino.h`

```cpp
extern "C" void tone(uint8_t pin, unsigned int frequency, unsigned long duration_ms = 0);
extern "C" void noTone(uint8_t pin);
```

### Step 3: Implement in a new `src/platforms/arduino_esp32/tone.cpp`

```cpp
#include <Arduino.h>
#include <esp32sim/clock.h>
#include <esp32sim/gpio.h>

extern "C" {
void tone(uint8_t pin, unsigned int frequency, unsigned long duration_ms) {
    // ... schedule alternating set_level calls via VirtualClock::schedule_at
}
void noTone(uint8_t pin) { /* ... cancel scheduled callbacks for this pin */ }
}  // extern "C"
```

### Step 4: Write the tests *first* (TDD per AGENTS.md)

`test/test_tone/test_tone.cpp`:

```cpp
#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void)    { esp32sim::Sim::reset(); }
void tearDown(void) {}

void test_tone_produces_square_wave_at_frequency(void) {
    pinMode(5, OUTPUT);
    tone(5, /*1 kHz*/ 1000, /*duration ms*/ 10);
    // After 0.5 ms, pin should toggle (1 kHz period = 1 ms = 0.5 ms half-period).
    esp32sim::Sim::advanceUs(500);
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(5).level());
    esp32sim::Sim::advanceUs(500);
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(5).level());
    // After total 10 ms, tone() stops.
    esp32sim::Sim::advanceMs(10);
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(5).level());
}
```

### Step 5: Iterate test → impl → test until green

Run `pio test -e native --filter test_tone`. Add tests for: noTone, frequency=0 edge case,
calling tone() while a previous tone is active, etc. Each becomes one TDD cycle.

## Conventions

- **Use `extern "C"` linkage** for everything in `Arduino.h` so consumer C/C++ sketches link
  identically to real arduino-esp32.
- **Match arduino-esp32 signatures exactly** including default parameters and return types.
  When in doubt, check the upstream `arduino-esp32` source.
- **Emit events** for any observable action (`EventLog::instance().emit(...)`) so test
  authors can assert via the event log without polling.
- **Document fidelity gaps** in `docs/user/explanation/what-this-does-and-doesnt-catch.md`
  if the behavior diverges from real hardware.
- **Update the supported-APIs reference**: `docs/user/reference/supported-arduino-apis.md`
  in the same commit series that lands the new API.

## Commit shape

One commit per logical step (TDD cycle). Example sequence:

```
feat(platforms): tone()/noTone() — basic square-wave generation
feat(platforms): noTone cancels in-flight tone()
feat(platforms): tone() with duration=0 plays until noTone
docs: tone()/noTone() in supported-arduino-apis.md
```

Each commit should run `pio test -e native --filter test_tone` green.
