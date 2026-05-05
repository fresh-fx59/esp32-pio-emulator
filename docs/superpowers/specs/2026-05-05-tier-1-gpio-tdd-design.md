# Tier 1 — GPIO TDD

| | |
|---|---|
| **Status** | v0.2 — refreshed at T1 entry; implementation-ready |
| **Date** | 2026-05-05 (v0.1), refreshed 2026-05-05 (v0.2) |
| **Parent** | [Master design](2026-05-05-esp32-pio-emulator-master-design.md) (v0.3) |
| **Depends on** | [Tier 0](2026-05-05-tier-0-skeleton-design.md) shipped (v0.1.0) |
| **Confidence** | High — implementation-ready |

## Goal

A developer can `pio test -e native` and write Unity tests against an unmodified Arduino-framework sketch that uses `pinMode`, `digitalWrite`, `digitalRead`, `Serial`, `millis`, `micros`, `delay`. The sketch's `setup()` and `loop()` are invoked by the test harness; tests assert on what the sketch did. Test feedback is sub-second.

After Tier 1, the value proposition "TDD for ESP32" is real for any sketch whose hardware footprint is GPIO + UART + timing.

## Changelog

| Date | Version | Change |
|---|---|---|
| 2026-05-05 | v0.1 | Initial draft during brainstorming, alongside master + T0–T4 specs. |
| 2026-05-05 | v0.2 | Refreshed at T1 entry. (a) File paths updated to master spec v0.3 layout (top-level `src/` and `include/`). (b) Open questions Q-T1-1..3 resolved per AGENTS.md decide-under-uncertainty. (c) `int main` instead of `extern "C" void app_main` to match host-native entry point and T0 pattern. (d) Coverage measurement requirement clarified (at least manually, before sign-off). (e) §"T0 learnings carried into T1" added to capture spec drift sources. |

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

The Unity-side test API exported by `include/esp32sim_unity/esp32sim.h` (per master spec v0.3 layout). This is what users write tests against.

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

int main(int /*argc*/, char** /*argv*/) {
  UNITY_BEGIN();
  RUN_TEST(test_led_blinks_on_pin_2_at_1hz);
  return UNITY_END();
}
```

(`int main` rather than `app_main` matches the T0 skeleton-test pattern and what
PlatformIO's `[env:native]` actually links against on Linux/macOS hosts. Anonymous
parameters silence `-Wextra`'s unused-parameter warning.)

The user's `setup()` and `loop()` are linked into the test binary unchanged.

## Implementation steps

Each step = one commit (or a tightly coupled mini-series), ends with verification + doc update per AGENTS.md. File paths reflect master-spec v0.3 layout (top-level `src/` and `include/`).

1. **`include/esp32sim/clock.h` + `src/core/clock.cpp` — VirtualClock.** Header-only `now_us()`, `advance(uint64_t us)`, `schedule_at(timestamp, callback)`, `cancel(handle)`. No Arduino dependency. Test: `test/test_core_clock/test_clock.cpp`. Verify: `pio test -e native --filter test_core_clock` green.
2. **`include/esp32sim/event_log.h` + `src/core/event_log.cpp` — EventLog.** `Event` struct, `EventKind` enum (start with GPIO_WRITE, GPIO_PINMODE, UART_TX, UART_RX), append + query-by-kind + query-by-time-range. Test: `test/test_core_event_log/`. Verify green.
3. **`include/esp32sim/gpio.h` + `src/core/pin_registry.cpp` — PinRegistry.** Pin level + mode + change-listeners. Test: `test/test_core_pin_registry/`. Verify green.
4. **`include/esp32sim/uart.h` + `src/core/uart_channel.cpp` — UartChannel.** TX buffer (queryable as `std::string`), RX buffer (injectable), per-UART. Test: `test/test_core_uart/`. Verify green.
5. **`include/Arduino.h` + `src/platforms/arduino_esp32/arduino.cpp`.** Header at root of `include/` (load-bearing — see master spec v0.3 §6). Defines `HIGH`/`LOW`/`INPUT`/`OUTPUT`/`INPUT_PULLUP`/`INPUT_PULLDOWN`, `pinMode`, `digital{Read,Write}`, `millis`, `micros`, `delay`, `delayMicroseconds`, `yield()` (no-op). Each function thin-forwards into `core/`. Test: a sketch-shaped test that calls these via `<Arduino.h>` and asserts via `esp32sim::Sim::*`. Verify green.
6. **`include/HardwareSerial.h` + `src/platforms/arduino_esp32/hardware_serial.cpp`.** `HardwareSerial` class with `begin`, `print`, `println`, `printf`, `write`, `read`, `available`, `peek`, `flush`. Globals `Serial`, `Serial1`, `Serial2` in their own translation unit. Test: a sketch that does `Serial.println("hello")` and asserts `esp32sim::Sim::uart(0).drainTx() == "hello\n"`. Verify green.
7. **`include/esp32sim_unity/esp32sim.h` + `src/harness/unity/sim.cpp` — ESP32Sim::* C++ API.** Implements the API spec above. Test: `test/test_harness_sim/` exercises every method (GpioRef, UartRef, EventQuery, time control, lifecycle). Verify green.
8. **`include/Arduino.h` `attachInterrupt` extension + `src/platforms/arduino_esp32/interrupts.cpp`.** Synchronous-fire model. RISING / FALLING / CHANGE / ONLOW / ONHIGH modes per arduino-esp32. Test: `test/test_attach_interrupt/`. Verify green.
9. **End-to-end example: `examples/01-blink/`.** Classic 1Hz blink sketch + Unity test. Sketch lives at `examples/01-blink/src/main.cpp` with its own `platformio.ini` lib_deps-ing the parent. Verify: example builds and tests pass; commit message says "first end-to-end TDD loop works".
10. **End-to-end example: `examples/02-button-debounce/`.** Debouncer sketch with `Sim::gpio(N).pulse(...)` injecting bouncing transitions; test asserts exactly one debounced output edge. Verify green.
11. **End-to-end example: `examples/03-serial-echo/`.** Reads from `Serial`, echoes back; test injects with `Sim::uart(0).inject(...)`, drains TX, asserts. Verify green.
12. **Documentation:**
    - `docs/user/tutorials/your-first-test.md` — 5-minute walkthrough using the blink example.
    - `docs/user/reference/sim-api.md` — full `esp32sim::Sim::*` API reference.
    - `docs/user/reference/supported-arduino-apis.md` — table of which Arduino calls work in T1.
    - `docs/user/explanation/why-virtual-time.md` — design rationale for the virtual clock.
    - `docs/user/explanation/what-this-does-and-doesnt-catch.md` — honest fidelity limits.
    - `docs/user/explanation/our-framework-vs-arduinofake.md` — per ADR-0003.
    - `docs/dev/how-to/add-a-new-arduino-api.md` — pattern for adding more Arduino functions.
    - `docs/user/how-to/test-an-interrupt-driven-sketch.md`.
    - All linked from `docs/README.md`. Diátaxis subfolder READMEs updated to remove the "(empty until T1)" notes for the docs that now exist.
13. **Tier 1 sign-off:** CHANGELOG bumped 0.1.0 → 0.2.0 with full T1 release notes; README status table flipped T1 ✓ / T2 🚧; CLAUDE.md "current tier" → "T2 starting"; consumer-side smoke test (a scratch project that includes a sketch and runs Unity tests against the sim) green; CI green; tag `v0.2.0` pushed.

## Verification gate (Tier 1 acceptance)

T1 ships when:

- All 13 steps committed and pushed.
- The three reference examples (blink, debounce, serial-echo) pass in CI.
- A new contributor following `docs/user/tutorials/your-first-test.md` reaches "first test passes" in under 10 minutes from a fresh clone.
- The framework's own `test/` suite has ≥80% line coverage of `src/core/` and `src/platforms/arduino_esp32/` (measured via `gcov` or equivalent on the `native` build). *Note: coverage tooling lands as a follow-up step in this tier; if it slips to T2 entry that's acceptable, but coverage NUMBER must be measured before T1 sign-off, even if reported manually.*
- README's "What works today" table reflects T1 reality.

## Out-of-scope behaviors (clarify what tests will NOT catch)

To set user expectations honestly:

- **Race conditions arising from real ESP32 dual-core preemption.** The fake runs single-threaded; ISRs fire synchronously. Real-hardware concurrency bugs may slip through.
- **Timing precision below microsecond.** The virtual clock is integer microseconds.
- **Voltage/current/electrical effects.** Fakes are digital. A pin is HIGH or LOW; we don't model rise time, capacitance, or short-circuits.
- **Compiler / toolchain differences.** Code is compiled with the host's compiler, not xtensa-esp32-elf-gcc. Architecture-specific bugs (e.g., misaligned access on Xtensa) won't reproduce.

`docs/user/explanation/what-this-does-and-doesnt-catch.md` documents this.

## Resolved decisions (T1)

(All Q-T1-* resolved per AGENTS.md decide-under-uncertainty in autonomous mode.)

- **Q-T1-1 → Resolved:** `delay()` inside `loop()` advances the virtual clock synchronously and returns; `runLoop()` returns when the user's `loop()` returns. Test authors advance time across loop iterations explicitly via `Sim::advanceMs()`. Critique: in real hardware, `delay()` may yield to FreeRTOS tasks; we don't simulate that in T1 (no scheduler). Mitigation: documented in `what-this-does-and-doesnt-catch.md`; T4 introduces RTOS shim where this matters.
- **Q-T1-2 → Resolved:** `Sim::runUntil(predicate, timeoutMs)` semantics: call `loop()` once, advance virtual time by 1 ms, check predicate, repeat until predicate is true or `timeoutMs` of virtual time has elapsed. Predictable; documented. Critique: doesn't model ISR-driven wakeups precisely. Mitigation: ISRs in T1 fire synchronously at the moment a pin level changes (between `loop()` iterations), so `runUntil` already covers ISR-driven scenarios as long as the level change happens via the test API.
- **Q-T1-3 → Resolved:** Unity only. No GoogleTest. Unity is PlatformIO's default; the macro set is sufficient for our needs; adding GoogleTest doubles test-runner surface area for marginal benefit. Critique: GoogleTest has richer mocking (gMock); we don't need that — our framework provides behavioral fakes. Revisit only if user feedback demands.

## T0 learnings carried into T1 (changes vs spec v0.1)

- **Layout:** moved from `core/include/`, `core/src/`, `platforms/arduino-esp32/include/`, etc. to top-level `include/` and `src/` with subdirectories — see master spec v0.3.
- **Test-binary entry point:** `int main(int /*argc*/, char** /*argv*/)` matches the T0 skeleton pattern and silences `-Wextra` unused-parameter warnings. (Earlier draft used `extern "C" void app_main()` which is the ESP-IDF entry point, not the host one.)
- **macOS not in CI yet:** if T1 introduces any platform-sensitive code (e.g., file paths, `clock_gettime` variants, `printf` width flags), the macOS-13 job goes back into the matrix in the same commit. Guard against this drift.
- **No ArduinoFake in `lib_deps`:** per ADR-0003, our `[env:native]` does not pull ArduinoFake. Tests use only Unity + our own headers/sources. The "explanation" doc clarifies the conceptual distinction for users who care.
- **Dev environment uses project-local `.venv/`:** the `setting-up-dev-environment.md` tutorial already documents this (T0 step 7); no change needed.
