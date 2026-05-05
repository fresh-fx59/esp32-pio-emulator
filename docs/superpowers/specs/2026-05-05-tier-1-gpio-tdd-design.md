# Tier 1 — GPIO TDD

| | |
|---|---|
| **Status** | Draft v0.1 — pending user review |
| **Date** | 2026-05-05 |
| **Parent** | [Master design](2026-05-05-esp32-pio-emulator-master-design.md) |
| **Depends on** | [Tier 0](2026-05-05-tier-0-skeleton-design.md) shipped |
| **Confidence** | High — implementation-ready |

## Goal

A developer can `pio test -e native` and write Unity tests against an unmodified Arduino-framework sketch that uses `pinMode`, `digitalWrite`, `digitalRead`, `Serial`, `millis`, `micros`, `delay`. The sketch's `setup()` and `loop()` are invoked by the test harness; tests assert on what the sketch did. Test feedback is sub-second.

After Tier 1, the value proposition "TDD for ESP32" is real for any sketch whose hardware footprint is GPIO + UART + timing.

## Scope

### In — APIs that work behaviorally

| Arduino API | Behavior in sim |
|---|---|
| `pinMode(pin, mode)` | Records mode in `PinRegistry`. INPUT, OUTPUT, INPUT_PULLUP, INPUT_PULLDOWN. |
| `digitalWrite(pin, level)` | Sets the pin's level in `PinRegistry`; emits `GPIO_WRITE` event; runs any change-listeners (e.g., a fake button watching the pin). |
| `digitalRead(pin)` | Returns the level set by `digitalWrite` *or* by a test-attached input source. |
| `Serial`, `Serial1`, `Serial2` (`HardwareSerial`) | `begin`, `print`, `println`, `printf`, `write`, `read`, `available`, `peek`, `flush`. Backed by `UartChannel`. |
| `millis()`, `micros()` | Return `VirtualClock.now_us() / 1000` and `now_us()`. |
| `delay(ms)`, `delayMicroseconds(us)` | Advance the virtual clock; do not sleep. |
| `yield()` | No-op in T1 (T4 makes it a scheduler hint). |
| `attachInterrupt(pin, isr, mode)` | Records ISR; fires it when the pin's level changes in the matching direction. **No real preemption** — ISR runs synchronously at the moment the level change happens, between sketch operations. |

### Out
- I2C, SPI, ADC, PWM, hardware timers — Tier 2.
- WiFi, networking — Tier 3.
- Filesystem, NVS, RTOS — Tier 4.
- The pytest-embedded harness — Tier 2.
- Multi-core simulation — Tier 4. T1 runs on a single simulated core.

## ESP32Sim assertion API

The Unity-side test API exported by `harness/unity/include/esp32sim_unity/esp32sim.h`. This is what users write tests against.

```cpp
namespace esp32sim {

class Sim {
public:
  // Lifecycle
  static void reset();              // resets all engine state
  static void runSetup();           // calls user's setup()
  static void runLoop(int n = 1);   // calls user's loop() n times
  static void runUntil(std::function<bool()> predicate, uint64_t timeoutMs);

  // Time
  static void advanceMs(uint64_t ms);
  static void advanceUs(uint64_t us);
  static uint64_t nowMs();

  // GPIO
  class GpioRef {
  public:
    int level() const;
    Mode mode() const;
    void setLevel(int);            // simulates an external pin driver
    void pulse(int level, uint64_t durationMs); // for buttons, etc.
  };
  static GpioRef gpio(uint8_t pin);

  // Serial / UART
  class UartRef {
  public:
    std::string drainTx();         // reads everything Serial.print()ed since last call
    std::string txAll() const;     // entire TX history
    bool txContains(std::string_view) const;
    void inject(std::string_view); // simulates incoming bytes; sketch's Serial.read() will see them
  };
  static UartRef uart(int n = 0);  // uart(0) = Serial

  // Events / log
  class EventQuery {
  public:
    EventQuery& kind(EventKind);
    EventQuery& pin(uint8_t);
    EventQuery& after(uint64_t timestampMs);
    size_t count() const;
    std::vector<Event> all() const;
  };
  static EventQuery events();
};

} // namespace esp32sim
```

### Example test (target experience)

```cpp
// test/test_blink/test_blink.cpp
#include <Arduino.h>
#include <unity.h>
#include <esp32sim_unity/esp32sim.h>

extern void setup();
extern void loop();

void test_led_blinks_on_pin_2_at_1hz() {
  esp32sim::Sim::reset();
  esp32sim::Sim::runSetup();

  TEST_ASSERT_EQUAL(LOW, esp32sim::Sim::gpio(2).level());

  esp32sim::Sim::runLoop(); // first iteration
  esp32sim::Sim::advanceMs(500);
  TEST_ASSERT_EQUAL(HIGH, esp32sim::Sim::gpio(2).level());

  esp32sim::Sim::runLoop();
  esp32sim::Sim::advanceMs(500);
  TEST_ASSERT_EQUAL(LOW, esp32sim::Sim::gpio(2).level());
}

extern "C" void app_main() {
  UNITY_BEGIN();
  RUN_TEST(test_led_blinks_on_pin_2_at_1hz);
  UNITY_END();
}
```

The user's `setup()` and `loop()` are linked into the test binary unchanged.

## Implementation steps

Each step = one commit (or a tightly coupled mini-series), ends with verification + doc update per AGENTS.md.

1. **`core/VirtualClock`** — header + impl + unit tests against direct C++ usage. No Arduino dependency. Verify: `pio test -e native --filter test_core_clock` green.
2. **`core/EventLog`** — append, query-by-kind, query-by-time-range. Unit tests. Verify: `--filter test_core_eventlog` green.
3. **`core/PinRegistry`** — pin level + mode + listeners. Unit tests. Verify: `--filter test_core_pin_registry` green.
4. **`core/UartChannel`** — TX buffer (queryable), RX buffer (injectable). Unit tests. Verify: `--filter test_core_uart` green.
5. **`platforms/arduino-esp32/Arduino.h`** — header that defines `HIGH`, `LOW`, `INPUT`, `OUTPUT`, `pinMode`, `digital{Read,Write}`, `millis`, `micros`, `delay`, `delayMicroseconds`. Each function calls into `core/`. Verify: a 10-line sketch compiles in `native` env.
6. **`platforms/arduino-esp32/HardwareSerial`** — `Serial`, `Serial1`, `Serial2` global instances; `begin`/`print`/`println`/`printf`/`write`/`read`/`available`/`peek`/`flush`. Verify: a sketch that does `Serial.println("hello")` produces "hello" in `Sim::uart(0).drainTx()`.
7. **`harness/unity/esp32sim_unity`** — `Sim::*` API as specified above. Each method has its own focused test. Verify: `--filter test_harness_*` green.
8. **`platforms/arduino-esp32/attachInterrupt`** — synchronous fire on level change. Unit tests cover RISING, FALLING, CHANGE. Verify: `--filter test_attach_interrupt` green.
9. **End-to-end example: `examples/01-blink/`** — classic blink sketch + a Unity test that proves the pin toggles correctly. Verify: example builds and tests pass; commit message says "first end-to-end TDD loop works".
10. **End-to-end example: `examples/02-button-debounce/`** — debouncer sketch with `Sim::gpio(N).pulse()` injecting bouncing transitions; test asserts exactly one debounced output edge. Verify: example builds and tests pass.
11. **End-to-end example: `examples/03-serial-echo/`** — sketch reads from `Serial`, echoes back; test injects with `Sim::uart(0).inject(...)`, drains TX, asserts. Verify: example builds and tests pass.
12. **Documentation:**
    - `docs/user/tutorials/your-first-test.md` — 5-minute walkthrough using the blink example.
    - `docs/user/reference/sim-api.md` — full `Sim::*` API reference.
    - `docs/user/explanation/why-virtual-time.md` — design rationale for the virtual clock.
    - `docs/dev/how-to/add-a-new-arduino-api.md` — pattern for adding more Arduino functions.
    - `docs/user/how-to/test-an-interrupt-driven-sketch.md`.
    - All linked from `docs/README.md`.
13. **Tier 1 sign-off:** CHANGELOG entry, README status update, `CLAUDE.md` "current tier" → "Tier 2 starting", final CI run green.

## Verification gate (Tier 1 acceptance)

T1 ships when:

- All 13 steps committed and pushed.
- The three reference examples (blink, debounce, serial-echo) pass in CI.
- A new contributor following `docs/user/tutorials/your-first-test.md` reaches "first test passes" in under 10 minutes from a fresh clone.
- The framework's own `test/` suite has ≥80% line coverage of `core/` and `platforms/arduino-esp32/` (measured via `gcov` or equivalent on the `native` build).
- README's "What works today" table reflects T1 reality.

## Out-of-scope behaviors (clarify what tests will NOT catch)

To set user expectations honestly:

- **Race conditions arising from real ESP32 dual-core preemption.** The fake runs single-threaded; ISRs fire synchronously. Real-hardware concurrency bugs may slip through.
- **Timing precision below microsecond.** The virtual clock is integer microseconds.
- **Voltage/current/electrical effects.** Fakes are digital. A pin is HIGH or LOW; we don't model rise time, capacitance, or short-circuits.
- **Compiler / toolchain differences.** Code is compiled with the host's compiler, not xtensa-esp32-elf-gcc. Architecture-specific bugs (e.g., misaligned access on Xtensa) won't reproduce.

`docs/user/explanation/what-this-does-and-doesnt-catch.md` documents this.

## Open questions (T1-specific)

- **Q-T1-1:** How does `Sim::runLoop()` interact with `delay()` *inside* `loop()`? *Default: `delay()` advances virtual clock synchronously; `runLoop()` returns when `loop()` returns. The test author advances time across loop iterations explicitly.*
- **Q-T1-2:** Should `Sim::runUntil(predicate, timeoutMs)` step `loop()` at a fixed rate, or every advance of virtual time, or every event? *Default: call `loop()`, advance 1 ms, check predicate, repeat. Document this; it's predictable but not realistic for ISR-driven code — that's why T1 ISRs are synchronous.*
- **Q-T1-3:** Should we provide GoogleTest as an alternative to Unity? *Default: no, Unity is PlatformIO's default and Unity has enough macros. Revisit if user feedback demands it.*
