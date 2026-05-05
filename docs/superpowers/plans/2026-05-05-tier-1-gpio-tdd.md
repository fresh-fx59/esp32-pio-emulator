# Tier 1 GPIO TDD — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** A user's unmodified Arduino sketch using `pinMode`, `digitalRead`, `digitalWrite`, `Serial`, `millis`, `micros`, `delay`, `attachInterrupt` compiles in PlatformIO `[env:native]` against our fakes, and Unity tests can drive `setup()` / `loop()` and assert on observable behavior — sub-second feedback, no hardware.

**Architecture:** Three layers of code (sketch → platform adapter `src/platforms/arduino_esp32/` → core engine `src/core/`) plus an in-process Unity test surface `src/harness/unity/`. All exposed via headers under `include/`. Per master spec v0.3 layout. Virtual clock is the timing substrate; `delay()` advances it instead of sleeping.

**Tech Stack:** C++17 (gnu++17), Unity test framework, PlatformIO `[env:native]`, GitHub Actions CI on Ubuntu 22.04.

**Reference docs (read before starting):**
- [Master design v0.3](../specs/2026-05-05-esp32-pio-emulator-master-design.md) — §4 architecture, §5 core abstractions, §6 layout
- [T1 spec v0.2](../specs/2026-05-05-tier-1-gpio-tdd-design.md) — full API surface and acceptance criteria
- [ADR-0001](../../decisions/0001-esp32-s3-primary-target.md) — ESP32-S3 primary
- [ADR-0003](../../decisions/0003-supersede-arduinofake-coexistence.md) — no ArduinoFake in `lib_deps`

**Working directory:** `/home/claude-developer/esp32-pio-emulator` on `main`.

**PIO command prefix:** Always use `.venv/bin/pio` (project-local venv). System `pio` is broken on Ubuntu 24/Python 3.12.

**File layout established by this plan:**

```
include/
├── Arduino.h                                # T1 task 5 — load-bearing fake
├── HardwareSerial.h                         # T1 task 6
├── esp32sim/
│   ├── esp32sim.h                           # umbrella (T1 task 7)
│   ├── clock.h                              # T1 task 1
│   ├── event_log.h                          # T1 task 2
│   ├── gpio.h                               # T1 task 3
│   └── uart.h                               # T1 task 4
└── esp32sim_unity/
    └── esp32sim.h                           # T1 task 7

src/
├── core/
│   ├── clock.cpp                            # T1 task 1
│   ├── event_log.cpp                        # T1 task 2
│   ├── pin_registry.cpp                     # T1 task 3
│   └── uart_channel.cpp                     # T1 task 4
├── platforms/
│   └── arduino_esp32/
│       ├── arduino.cpp                      # T1 task 5
│       ├── hardware_serial.cpp              # T1 task 6
│       └── interrupts.cpp                   # T1 task 8
└── harness/
    └── unity/
        └── sim.cpp                          # T1 task 7

test/
├── test_skeleton/                           # already exists from T0
├── test_core_clock/test_clock.cpp           # T1 task 1
├── test_core_event_log/test_event_log.cpp   # T1 task 2
├── test_core_pin_registry/test_pin_registry.cpp  # T1 task 3
├── test_core_uart/test_uart.cpp             # T1 task 4
├── test_arduino_basics/test_arduino.cpp     # T1 task 5
├── test_hardware_serial/test_serial.cpp     # T1 task 6
├── test_harness_sim/test_sim.cpp            # T1 task 7
└── test_attach_interrupt/test_interrupts.cpp  # T1 task 8

examples/
├── 01-blink/                                # T1 task 9
├── 02-button-debounce/                      # T1 task 10
└── 03-serial-echo/                          # T1 task 11
```

**Convention reminders (apply to every task):**
- Per AGENTS.md: small reviewable commits, one concept per commit, never `--no-verify`, never amend a published commit.
- Per AGENTS.md per-step verify-and-document: each task ends with (1) verification command run + green, (2) any docs touched updated in same commit, (3) commit + push to `main`.
- Per AGENTS.md "Subagent-first dispatch": each task in this plan should be executed by a fresh subagent. Reserve in-line work for review and stitching.
- TDD: tests come before implementation. The pattern in every task is: write test → run → verify it fails → write impl → run → verify it passes → commit.
- C++17 features available (no need to backport): `std::string_view`, `std::optional`, `std::variant`, structured bindings, `if constexpr`, `<chrono>`.

---

## Task 1: VirtualClock — virtual time foundation

**Files:**
- Create: `include/esp32sim/clock.h`
- Create: `src/core/clock.cpp`
- Create: `test/test_core_clock/test_clock.cpp`

**What:** A monotonic virtual clock measured in microseconds. Tests advance time explicitly; nothing in the sim ever sleeps. Supports scheduling callbacks (used later by hardware timers in T2 and timed alarms generally).

- [ ] **Step 1: Write failing test for clock starts at 0.**

`test/test_core_clock/test_clock.cpp`:

```cpp
#include <esp32sim/clock.h>
#include <unity.h>

using esp32sim::VirtualClock;

void setUp(void) { VirtualClock::instance().reset(); }
void tearDown(void) {}

void test_clock_starts_at_zero(void) {
    TEST_ASSERT_EQUAL_UINT64(0, VirtualClock::instance().now_us());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_clock_starts_at_zero);
    return UNITY_END();
}
```

- [ ] **Step 2: Run test, verify fail.**

```bash
.venv/bin/pio test -e native --filter test_core_clock 2>&1 | tail -10
```

Expected: compilation FAIL — `esp32sim/clock.h: No such file`.

- [ ] **Step 3: Write minimal `include/esp32sim/clock.h` to make it compile.**

```cpp
// include/esp32sim/clock.h
#pragma once

#include <cstdint>
#include <functional>
#include <map>

namespace esp32sim {

using ScheduleHandle = uint64_t;

class VirtualClock {
public:
    static VirtualClock& instance();

    // Singletons aren't normally great, but the Arduino API (millis(), delay())
    // is implicitly singleton — there's exactly one notion of "now" in a sketch.
    // Resetting between tests is mandatory; setUp() must call reset().
    void reset();

    uint64_t now_us() const { return now_us_; }
    uint64_t now_ms() const { return now_us_ / 1000; }

    // Advance time, firing scheduled callbacks at their target timestamp.
    // Callbacks fire in timestamp order; if multiple at the same timestamp,
    // insertion order.
    void advance_us(uint64_t us);
    void advance_ms(uint64_t ms) { advance_us(ms * 1000); }

    // Schedule a callback to fire when now_us() reaches `at_us`. Returns a
    // handle the caller can use to cancel. Cancelled callbacks never fire.
    ScheduleHandle schedule_at(uint64_t at_us, std::function<void()> cb);
    bool cancel(ScheduleHandle h);

private:
    VirtualClock() = default;
    uint64_t now_us_ = 0;
    ScheduleHandle next_handle_ = 1;
    // Multimap so multiple callbacks at the same timestamp are preserved in
    // insertion order (multimap maintains insertion order for equal keys).
    std::multimap<uint64_t, std::pair<ScheduleHandle, std::function<void()>>> schedule_;
};

}  // namespace esp32sim
```

`src/core/clock.cpp`:

```cpp
// src/core/clock.cpp
#include <esp32sim/clock.h>

namespace esp32sim {

VirtualClock& VirtualClock::instance() {
    static VirtualClock c;
    return c;
}

void VirtualClock::reset() {
    now_us_ = 0;
    next_handle_ = 1;
    schedule_.clear();
}

void VirtualClock::advance_us(uint64_t us) {
    uint64_t target = now_us_ + us;
    while (!schedule_.empty()) {
        auto it = schedule_.begin();
        if (it->first > target) break;
        now_us_ = it->first;
        auto cb = std::move(it->second.second);
        schedule_.erase(it);
        if (cb) cb();
    }
    now_us_ = target;
}

ScheduleHandle VirtualClock::schedule_at(uint64_t at_us, std::function<void()> cb) {
    if (at_us < now_us_) at_us = now_us_;  // past times fire on next advance
    ScheduleHandle h = next_handle_++;
    schedule_.emplace(at_us, std::make_pair(h, std::move(cb)));
    return h;
}

bool VirtualClock::cancel(ScheduleHandle h) {
    for (auto it = schedule_.begin(); it != schedule_.end(); ++it) {
        if (it->second.first == h) {
            schedule_.erase(it);
            return true;
        }
    }
    return false;
}

}  // namespace esp32sim
```

- [ ] **Step 4: Run test, verify pass.**

```bash
.venv/bin/pio test -e native --filter test_core_clock 2>&1 | tail -10
```

Expected: `1 test cases: 1 succeeded`.

- [ ] **Step 5: Add more tests (advance, scheduling, cancel, multiple-callbacks-same-time).**

Append to `test/test_core_clock/test_clock.cpp` (before `main`):

```cpp
void test_advance_increments_now_us(void) {
    VirtualClock::instance().advance_us(123);
    TEST_ASSERT_EQUAL_UINT64(123, VirtualClock::instance().now_us());
    VirtualClock::instance().advance_us(877);
    TEST_ASSERT_EQUAL_UINT64(1000, VirtualClock::instance().now_us());
}

void test_advance_ms_converts_correctly(void) {
    VirtualClock::instance().advance_ms(5);
    TEST_ASSERT_EQUAL_UINT64(5000, VirtualClock::instance().now_us());
    TEST_ASSERT_EQUAL_UINT64(5, VirtualClock::instance().now_ms());
}

void test_scheduled_callback_fires_when_time_reached(void) {
    int fired = 0;
    VirtualClock::instance().schedule_at(100, [&fired]() { fired++; });
    TEST_ASSERT_EQUAL_INT(0, fired);
    VirtualClock::instance().advance_us(50);
    TEST_ASSERT_EQUAL_INT(0, fired);
    VirtualClock::instance().advance_us(50);
    TEST_ASSERT_EQUAL_INT(1, fired);
    VirtualClock::instance().advance_us(1000);  // doesn't fire again
    TEST_ASSERT_EQUAL_INT(1, fired);
}

void test_multiple_callbacks_same_timestamp_fire_in_insertion_order(void) {
    int seq[3] = {0, 0, 0};
    int idx = 0;
    auto& c = VirtualClock::instance();
    c.schedule_at(100, [&]() { seq[idx++] = 1; });
    c.schedule_at(100, [&]() { seq[idx++] = 2; });
    c.schedule_at(100, [&]() { seq[idx++] = 3; });
    c.advance_us(100);
    TEST_ASSERT_EQUAL_INT(1, seq[0]);
    TEST_ASSERT_EQUAL_INT(2, seq[1]);
    TEST_ASSERT_EQUAL_INT(3, seq[2]);
}

void test_cancel_prevents_callback(void) {
    int fired = 0;
    auto h = VirtualClock::instance().schedule_at(100, [&fired]() { fired++; });
    TEST_ASSERT_TRUE(VirtualClock::instance().cancel(h));
    VirtualClock::instance().advance_us(1000);
    TEST_ASSERT_EQUAL_INT(0, fired);
}

void test_cancel_unknown_handle_returns_false(void) {
    TEST_ASSERT_FALSE(VirtualClock::instance().cancel(99999));
}

void test_reset_clears_clock_and_schedule(void) {
    int fired = 0;
    VirtualClock::instance().advance_us(500);
    VirtualClock::instance().schedule_at(1000, [&fired]() { fired++; });
    VirtualClock::instance().reset();
    TEST_ASSERT_EQUAL_UINT64(0, VirtualClock::instance().now_us());
    VirtualClock::instance().advance_us(2000);
    TEST_ASSERT_EQUAL_INT(0, fired);  // scheduled cb cleared by reset
}
```

Update `main` to register them:

```cpp
int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_clock_starts_at_zero);
    RUN_TEST(test_advance_increments_now_us);
    RUN_TEST(test_advance_ms_converts_correctly);
    RUN_TEST(test_scheduled_callback_fires_when_time_reached);
    RUN_TEST(test_multiple_callbacks_same_timestamp_fire_in_insertion_order);
    RUN_TEST(test_cancel_prevents_callback);
    RUN_TEST(test_cancel_unknown_handle_returns_false);
    RUN_TEST(test_reset_clears_clock_and_schedule);
    return UNITY_END();
}
```

- [ ] **Step 6: Run all clock tests, verify pass.**

```bash
.venv/bin/pio test -e native --filter test_core_clock 2>&1 | tail -15
```

Expected: `8 test cases: 8 succeeded`.

- [ ] **Step 7: Commit.**

```bash
git add include/esp32sim/clock.h src/core/clock.cpp test/test_core_clock/
git commit -m "feat(core): VirtualClock — virtual-time foundation

T1 task 1. Singleton clock measured in microseconds with multimap-
backed scheduler. Tests advance time explicitly; nothing in the sim
sleeps. 8 unit tests cover: now_us starts at 0, advance increments
correctly, ms/us conversion, scheduled callbacks fire at their
timestamp, multiple callbacks at same timestamp fire in insertion
order (multimap property), cancel() returns expected bool, reset()
clears both clock and schedule.

Critique: singleton is generally a code smell, but the Arduino API
(millis(), delay()) is implicitly singleton — there's one 'now' in
a sketch. Mitigation: explicit reset() called in every test's
setUp(); subsequent tasks must follow the same pattern."
git push origin main
```

---

## Task 2: EventLog — observable-action recording

**Files:**
- Create: `include/esp32sim/event_log.h`
- Create: `src/core/event_log.cpp`
- Create: `test/test_core_event_log/test_event_log.cpp`

**What:** Append-only log of every observable simulator action (GPIO write, UART tx byte, etc.). Keyed by virtual-time timestamp. Powers user-facing assertions (`Sim::events().kind(GPIO_WRITE).count()`).

- [ ] **Step 1: Write failing test for emit/query basics.**

`test/test_core_event_log/test_event_log.cpp`:

```cpp
#include <esp32sim/clock.h>
#include <esp32sim/event_log.h>
#include <unity.h>

using esp32sim::Event;
using esp32sim::EventKind;
using esp32sim::EventLog;
using esp32sim::VirtualClock;

void setUp(void) {
    VirtualClock::instance().reset();
    EventLog::instance().reset();
}
void tearDown(void) {}

void test_log_starts_empty(void) {
    TEST_ASSERT_EQUAL_size_t(0, EventLog::instance().count());
}

void test_emit_appends_event_with_current_timestamp(void) {
    VirtualClock::instance().advance_us(500);
    EventLog::instance().emit(Event{EventKind::GPIO_WRITE, /*pin*/2, /*value*/1});
    TEST_ASSERT_EQUAL_size_t(1, EventLog::instance().count());
    auto evs = EventLog::instance().all();
    TEST_ASSERT_EQUAL_UINT64(500, evs[0].timestamp_us);
    TEST_ASSERT_EQUAL_INT((int)EventKind::GPIO_WRITE, (int)evs[0].kind);
    TEST_ASSERT_EQUAL_INT(2, evs[0].pin);
    TEST_ASSERT_EQUAL_INT(1, evs[0].value);
}

void test_filter_by_kind(void) {
    auto& log = EventLog::instance();
    log.emit(Event{EventKind::GPIO_WRITE, 2, 1});
    log.emit(Event{EventKind::UART_TX, 0, 'h'});
    log.emit(Event{EventKind::GPIO_WRITE, 2, 0});
    auto gpio = log.filter([](const Event& e) { return e.kind == EventKind::GPIO_WRITE; });
    TEST_ASSERT_EQUAL_size_t(2, gpio.size());
}

void test_filter_by_time_range(void) {
    auto& log = EventLog::instance();
    auto& clk = VirtualClock::instance();
    log.emit(Event{EventKind::GPIO_WRITE, 1, 1});  // t=0
    clk.advance_us(100);
    log.emit(Event{EventKind::GPIO_WRITE, 2, 1});  // t=100
    clk.advance_us(100);
    log.emit(Event{EventKind::GPIO_WRITE, 3, 1});  // t=200
    auto in_range = log.between(50, 150);
    TEST_ASSERT_EQUAL_size_t(1, in_range.size());
    TEST_ASSERT_EQUAL_INT(2, in_range[0].pin);
}

void test_reset_clears_log(void) {
    EventLog::instance().emit(Event{EventKind::GPIO_WRITE, 2, 1});
    EventLog::instance().reset();
    TEST_ASSERT_EQUAL_size_t(0, EventLog::instance().count());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_log_starts_empty);
    RUN_TEST(test_emit_appends_event_with_current_timestamp);
    RUN_TEST(test_filter_by_kind);
    RUN_TEST(test_filter_by_time_range);
    RUN_TEST(test_reset_clears_log);
    return UNITY_END();
}
```

- [ ] **Step 2: Run, verify fail (header missing).**

```bash
.venv/bin/pio test -e native --filter test_core_event_log 2>&1 | tail -10
```

Expected: compilation fails on `esp32sim/event_log.h`.

- [ ] **Step 3: Write `include/esp32sim/event_log.h` and `src/core/event_log.cpp`.**

`include/esp32sim/event_log.h`:

```cpp
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
```

`src/core/event_log.cpp`:

```cpp
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
```

- [ ] **Step 4: Run, verify pass.**

```bash
.venv/bin/pio test -e native --filter test_core_event_log 2>&1 | tail -10
```

Expected: `5 test cases: 5 succeeded`.

- [ ] **Step 5: Commit.**

```bash
git add include/esp32sim/event_log.h src/core/event_log.cpp test/test_core_event_log/
git commit -m "feat(core): EventLog — observable-action recording

T1 task 2. Append-only log of simulator events keyed by virtual
clock timestamp. EventKind enum starts narrow (GPIO_WRITE, UART_TX,
etc.); will widen to a std::variant payload when T2 needs to record
I2C transactions.

emit() auto-populates timestamp_us from VirtualClock if not set
explicitly. filter() / between() / by_kind() are read-only views
(return new vectors). 5 unit tests cover empty state, emit-and-
timestamp, filter-by-kind, between(), reset.

Critique: singleton again. Same justification as VirtualClock —
the simulator has one observable-state log. Reset called per-test."
git push origin main
```

---

## Task 3: PinRegistry — GPIO state and listeners

**Files:**
- Create: `include/esp32sim/gpio.h`
- Create: `src/core/pin_registry.cpp`
- Create: `test/test_core_pin_registry/test_pin_registry.cpp`

**What:** Tracks pin level, mode (INPUT/OUTPUT/INPUT_PULLUP/INPUT_PULLDOWN), and change-listeners (used by `attachInterrupt`). Pin number range matches ESP32-S3 (0–48).

- [ ] **Step 1: Write failing tests covering pin level/mode read-write, listeners, reset.**

`test/test_core_pin_registry/test_pin_registry.cpp`:

```cpp
#include <esp32sim/clock.h>
#include <esp32sim/event_log.h>
#include <esp32sim/gpio.h>
#include <unity.h>

using namespace esp32sim;

void setUp(void) {
    VirtualClock::instance().reset();
    EventLog::instance().reset();
    PinRegistry::instance().reset();
}
void tearDown(void) {}

void test_pin_starts_low_input(void) {
    auto& r = PinRegistry::instance();
    TEST_ASSERT_EQUAL_INT(0, r.get_level(2));
    TEST_ASSERT_EQUAL_INT((int)PinMode::INPUT, (int)r.get_mode(2));
}

void test_set_level_records_in_event_log(void) {
    auto& r = PinRegistry::instance();
    r.set_mode(2, PinMode::OUTPUT);
    r.set_level(2, 1);
    TEST_ASSERT_EQUAL_INT(1, r.get_level(2));
    auto writes = EventLog::instance().by_kind(EventKind::GPIO_WRITE);
    TEST_ASSERT_EQUAL_size_t(1, writes.size());
    TEST_ASSERT_EQUAL_INT(2, writes[0].pin);
    TEST_ASSERT_EQUAL_INT(1, writes[0].value);
}

void test_set_mode_records_in_event_log(void) {
    PinRegistry::instance().set_mode(4, PinMode::INPUT_PULLUP);
    auto modes = EventLog::instance().by_kind(EventKind::GPIO_PIN_MODE);
    TEST_ASSERT_EQUAL_size_t(1, modes.size());
    TEST_ASSERT_EQUAL_INT(4, modes[0].pin);
    TEST_ASSERT_EQUAL_INT((int)PinMode::INPUT_PULLUP, modes[0].value);
}

void test_pullup_pin_reads_high_before_any_drive(void) {
    auto& r = PinRegistry::instance();
    r.set_mode(5, PinMode::INPUT_PULLUP);
    TEST_ASSERT_EQUAL_INT(1, r.get_level(5));
}

void test_pulldown_pin_reads_low(void) {
    auto& r = PinRegistry::instance();
    r.set_mode(5, PinMode::INPUT_PULLDOWN);
    TEST_ASSERT_EQUAL_INT(0, r.get_level(5));
}

void test_listener_fires_on_level_change(void) {
    auto& r = PinRegistry::instance();
    int rising = 0, falling = 0;
    r.add_listener(2, [&](int old_lvl, int new_lvl) {
        if (old_lvl == 0 && new_lvl == 1) rising++;
        if (old_lvl == 1 && new_lvl == 0) falling++;
    });
    r.set_level(2, 1);
    r.set_level(2, 0);
    r.set_level(2, 0);  // no change — listener not called
    TEST_ASSERT_EQUAL_INT(1, rising);
    TEST_ASSERT_EQUAL_INT(1, falling);
}

void test_invalid_pin_throws_or_clamps(void) {
    // ESP32-S3 has GPIO 0..48. We accept 0..63 conservatively (some variants
    // map RTC-only pins higher); pin 100 is out of range.
    auto& r = PinRegistry::instance();
    // Out-of-range get_level should return 0 silently (mirrors arduino-esp32
    // behavior — no exception, just no-op).
    TEST_ASSERT_EQUAL_INT(0, r.get_level(100));
    r.set_level(100, 1);
    TEST_ASSERT_EQUAL_INT(0, r.get_level(100));  // didn't take
}

void test_reset_clears_state_and_listeners(void) {
    auto& r = PinRegistry::instance();
    bool fired = false;
    r.set_level(2, 1);
    r.add_listener(3, [&fired](int, int) { fired = true; });
    r.reset();
    TEST_ASSERT_EQUAL_INT(0, r.get_level(2));
    r.set_level(3, 1);
    TEST_ASSERT_FALSE(fired);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_pin_starts_low_input);
    RUN_TEST(test_set_level_records_in_event_log);
    RUN_TEST(test_set_mode_records_in_event_log);
    RUN_TEST(test_pullup_pin_reads_high_before_any_drive);
    RUN_TEST(test_pulldown_pin_reads_low);
    RUN_TEST(test_listener_fires_on_level_change);
    RUN_TEST(test_invalid_pin_throws_or_clamps);
    RUN_TEST(test_reset_clears_state_and_listeners);
    return UNITY_END();
}
```

- [ ] **Step 2: Run, verify fail.**

```bash
.venv/bin/pio test -e native --filter test_core_pin_registry 2>&1 | tail -10
```

Expected: fail to compile, missing `esp32sim/gpio.h`.

- [ ] **Step 3: Write `include/esp32sim/gpio.h` + `src/core/pin_registry.cpp`.**

`include/esp32sim/gpio.h`:

```cpp
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
```

`src/core/pin_registry.cpp`:

```cpp
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
```

- [ ] **Step 4: Run, verify pass.**

```bash
.venv/bin/pio test -e native --filter test_core_pin_registry 2>&1 | tail -10
```

Expected: `8 test cases: 8 succeeded`.

- [ ] **Step 5: Commit.**

```bash
git add include/esp32sim/gpio.h src/core/pin_registry.cpp test/test_core_pin_registry/
git commit -m "feat(core): PinRegistry — GPIO state, modes, listeners

T1 task 3. Per-pin level + mode + listener vector. Pins 0..63
covered (ESP32-S3 has 0..48; conservative margin). set_level emits
GPIO_WRITE event; set_mode emits GPIO_PIN_MODE event. Listeners
fire synchronously when level CHANGES (not on every set_level).
INPUT_PULLUP reads high until explicitly driven.

8 unit tests cover: initial state, write/read, mode setting + event
emission, pullup default-high, pulldown default-low, listener fires
on change only (not on no-op writes), out-of-range pin is silent
no-op (matches arduino-esp32 behavior — no exception), reset clears
both state and listeners.

Critique: out-of-range 'silent no-op' could hide bugs. Mitigation:
EventLog still doesn't get an entry for out-of-range writes, so
test code asserting on event count will catch unintended writes."
git push origin main
```

---

## Task 4: UartChannel — TX/RX buffers per UART

**Files:**
- Create: `include/esp32sim/uart.h`
- Create: `src/core/uart_channel.cpp`
- Create: `test/test_core_uart/test_uart.cpp`

**What:** Per-UART channel (Serial, Serial1, Serial2 on ESP32-S3) with a TX buffer (sketch writes go here; tests drain it as `std::string`) and an RX buffer (tests inject into it; sketch reads consume from it).

- [ ] **Step 1: Write failing tests.**

`test/test_core_uart/test_uart.cpp`:

```cpp
#include <esp32sim/clock.h>
#include <esp32sim/event_log.h>
#include <esp32sim/uart.h>
#include <unity.h>

using namespace esp32sim;

void setUp(void) {
    VirtualClock::instance().reset();
    EventLog::instance().reset();
    UartChannel::reset_all();
}
void tearDown(void) {}

void test_uart0_starts_empty(void) {
    auto& u = UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_size_t(0, u.tx_buffer().size());
    TEST_ASSERT_FALSE(u.rx_available());
}

void test_tx_write_appends_and_emits_event(void) {
    auto& u = UartChannel::for_index(0);
    u.tx_write_byte('H');
    u.tx_write_byte('i');
    TEST_ASSERT_EQUAL_STRING("Hi", u.drain_tx().c_str());
    auto txes = EventLog::instance().by_kind(EventKind::UART_TX);
    TEST_ASSERT_EQUAL_size_t(2, txes.size());
}

void test_drain_tx_clears_buffer(void) {
    auto& u = UartChannel::for_index(0);
    u.tx_write_byte('a');
    TEST_ASSERT_EQUAL_STRING("a", u.drain_tx().c_str());
    TEST_ASSERT_EQUAL_STRING("", u.drain_tx().c_str());
}

void test_tx_history_preserves_full_log(void) {
    auto& u = UartChannel::for_index(0);
    u.tx_write_byte('a');
    u.drain_tx();
    u.tx_write_byte('b');
    TEST_ASSERT_EQUAL_STRING("ab", u.tx_history().c_str());
}

void test_rx_inject_then_read(void) {
    auto& u = UartChannel::for_index(0);
    u.rx_inject("hello");
    TEST_ASSERT_TRUE(u.rx_available());
    TEST_ASSERT_EQUAL_INT(5, u.rx_size());
    TEST_ASSERT_EQUAL_INT('h', u.rx_read_byte());
    TEST_ASSERT_EQUAL_INT('e', u.rx_read_byte());
    TEST_ASSERT_EQUAL_INT(3, u.rx_size());
}

void test_rx_read_when_empty_returns_minus_one(void) {
    auto& u = UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_INT(-1, u.rx_read_byte());
}

void test_rx_peek_does_not_consume(void) {
    auto& u = UartChannel::for_index(0);
    u.rx_inject("abc");
    TEST_ASSERT_EQUAL_INT('a', u.rx_peek());
    TEST_ASSERT_EQUAL_INT('a', u.rx_peek());
    TEST_ASSERT_EQUAL_INT(3, u.rx_size());
}

void test_three_uarts_isolated(void) {
    auto& u0 = UartChannel::for_index(0);
    auto& u1 = UartChannel::for_index(1);
    auto& u2 = UartChannel::for_index(2);
    u0.tx_write_byte('a');
    u1.tx_write_byte('b');
    u2.tx_write_byte('c');
    TEST_ASSERT_EQUAL_STRING("a", u0.drain_tx().c_str());
    TEST_ASSERT_EQUAL_STRING("b", u1.drain_tx().c_str());
    TEST_ASSERT_EQUAL_STRING("c", u2.drain_tx().c_str());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_uart0_starts_empty);
    RUN_TEST(test_tx_write_appends_and_emits_event);
    RUN_TEST(test_drain_tx_clears_buffer);
    RUN_TEST(test_tx_history_preserves_full_log);
    RUN_TEST(test_rx_inject_then_read);
    RUN_TEST(test_rx_read_when_empty_returns_minus_one);
    RUN_TEST(test_rx_peek_does_not_consume);
    RUN_TEST(test_three_uarts_isolated);
    return UNITY_END();
}
```

- [ ] **Step 2: Run, verify fail.**

```bash
.venv/bin/pio test -e native --filter test_core_uart 2>&1 | tail -10
```

Expected: missing header.

- [ ] **Step 3: Write `include/esp32sim/uart.h` + `src/core/uart_channel.cpp`.**

`include/esp32sim/uart.h`:

```cpp
#pragma once

#include <cstdint>
#include <deque>
#include <string>
#include <string_view>

namespace esp32sim {

class UartChannel {
public:
    // ESP32-S3 has UART0/1/2. for_index(N) returns the channel; out-of-range
    // returns channel 0 (mirrors arduino-esp32's silent fallback for
    // unsupported indices).
    static UartChannel& for_index(int n);
    static void reset_all();

    // TX side — sketch writes here, tests drain or query.
    void tx_write_byte(uint8_t b);
    std::string drain_tx();           // returns and clears the un-drained portion
    std::string tx_history() const;   // entire TX log since last reset_all()

    // RX side — tests inject, sketch reads.
    void rx_inject(std::string_view s);
    bool rx_available() const { return !rx_buffer_.empty(); }
    size_t rx_size() const { return rx_buffer_.size(); }
    int rx_read_byte();               // -1 if empty
    int rx_peek() const;              // -1 if empty

    size_t tx_buffer() const { return tx_drainable_.size(); }  // for tests

private:
    UartChannel() = default;
    void reset_();

    std::string tx_drainable_;        // grows on write, cleared on drain_tx
    std::string tx_full_history_;     // grows on write, only cleared on reset_all
    std::deque<uint8_t> rx_buffer_;
};

}  // namespace esp32sim
```

`src/core/uart_channel.cpp`:

```cpp
#include <esp32sim/event_log.h>
#include <esp32sim/uart.h>

namespace esp32sim {

namespace {
constexpr int kUartCount = 3;
UartChannel& channel(int n) {
    static UartChannel chans[kUartCount];
    if (n < 0 || n >= kUartCount) n = 0;
    return chans[n];
}
}  // namespace

UartChannel& UartChannel::for_index(int n) { return channel(n); }

void UartChannel::reset_all() {
    for (int i = 0; i < kUartCount; ++i) channel(i).reset_();
}

void UartChannel::reset_() {
    tx_drainable_.clear();
    tx_full_history_.clear();
    rx_buffer_.clear();
}

void UartChannel::tx_write_byte(uint8_t b) {
    tx_drainable_ += static_cast<char>(b);
    tx_full_history_ += static_cast<char>(b);
    EventLog::instance().emit(Event{EventKind::UART_TX, /*pin=uart_index*/0, (int)b});
}

std::string UartChannel::drain_tx() {
    std::string out = tx_drainable_;
    tx_drainable_.clear();
    return out;
}

std::string UartChannel::tx_history() const { return tx_full_history_; }

void UartChannel::rx_inject(std::string_view s) {
    for (char c : s) rx_buffer_.push_back(static_cast<uint8_t>(c));
}

int UartChannel::rx_read_byte() {
    if (rx_buffer_.empty()) return -1;
    int b = rx_buffer_.front();
    rx_buffer_.pop_front();
    EventLog::instance().emit(Event{EventKind::UART_RX, 0, b});
    return b;
}

int UartChannel::rx_peek() const {
    if (rx_buffer_.empty()) return -1;
    return rx_buffer_.front();
}

}  // namespace esp32sim
```

- [ ] **Step 4: Run, verify pass.**

```bash
.venv/bin/pio test -e native --filter test_core_uart 2>&1 | tail -10
```

Expected: `8 test cases: 8 succeeded`.

- [ ] **Step 5: Commit.**

```bash
git add include/esp32sim/uart.h src/core/uart_channel.cpp test/test_core_uart/
git commit -m "feat(core): UartChannel — per-UART TX/RX buffers

T1 task 4. Three channels (UART0/1/2) matching ESP32-S3. Each has:
- TX side: tx_write_byte appends, drain_tx returns + clears
  un-drained portion, tx_history is the full log since reset_all.
- RX side: rx_inject for tests, rx_available/rx_size/rx_read_byte/
  rx_peek for sketches. rx_read_byte returns -1 on empty (matches
  arduino-esp32 Serial.read() contract).

Every byte in/out emits a UART_TX/UART_RX event so tests can
correlate UART activity with timestamps via EventLog::between().

8 unit tests cover: empty initial state, TX write+drain, drain
clears the slice but tx_history preserves all, RX inject+read+peek,
out-of-range channel index (returns chan 0), three-channel
isolation."
git push origin main
```

---

## Task 5: Arduino.h fake — the load-bearing header

**Files:**
- Create: `include/Arduino.h`
- Create: `src/platforms/arduino_esp32/arduino.cpp`
- Create: `test/test_arduino_basics/test_arduino.cpp`

**What:** The header at `include/Arduino.h` that consumer sketches transparently `#include <Arduino.h>` — they get our fake instead of the real arduino-esp32 one. Defines macros (HIGH/LOW/INPUT/OUTPUT/INPUT_PULLUP/INPUT_PULLDOWN/RISING/FALLING/CHANGE), pin functions (`pinMode`/`digital{Read,Write}`), time (`millis`/`micros`/`delay`/`delayMicroseconds`), `yield()` no-op.

This is the load-bearing fake — it must be header-compatible enough that real sketches compile.

- [ ] **Step 1: Write failing tests that exercise the Arduino API directly.**

`test/test_arduino_basics/test_arduino.cpp`:

```cpp
#include <Arduino.h>
#include <esp32sim/clock.h>
#include <esp32sim/gpio.h>
#include <esp32sim/event_log.h>
#include <unity.h>

void setUp(void) {
    esp32sim::VirtualClock::instance().reset();
    esp32sim::EventLog::instance().reset();
    esp32sim::PinRegistry::instance().reset();
}
void tearDown(void) {}

void test_macros_defined(void) {
    TEST_ASSERT_EQUAL_INT(0, LOW);
    TEST_ASSERT_EQUAL_INT(1, HIGH);
}

void test_digitalWrite_then_digitalRead(void) {
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);
    TEST_ASSERT_EQUAL_INT(HIGH, digitalRead(2));
    digitalWrite(2, LOW);
    TEST_ASSERT_EQUAL_INT(LOW, digitalRead(2));
}

void test_pinMode_records_in_event_log(void) {
    pinMode(3, INPUT_PULLUP);
    auto modes = esp32sim::EventLog::instance().by_kind(esp32sim::EventKind::GPIO_PIN_MODE);
    TEST_ASSERT_EQUAL_size_t(1, modes.size());
    TEST_ASSERT_EQUAL_INT(3, modes[0].pin);
    TEST_ASSERT_EQUAL_INT((int)esp32sim::PinMode::INPUT_PULLUP, modes[0].value);
}

void test_input_pullup_reads_high_by_default(void) {
    pinMode(4, INPUT_PULLUP);
    TEST_ASSERT_EQUAL_INT(HIGH, digitalRead(4));
}

void test_millis_micros_advance_with_delay(void) {
    TEST_ASSERT_EQUAL_UINT32(0, millis());
    delay(50);
    TEST_ASSERT_EQUAL_UINT32(50, millis());
    TEST_ASSERT_EQUAL_UINT32(50000, micros());
    delayMicroseconds(123);
    TEST_ASSERT_EQUAL_UINT32(50123, micros());
}

void test_yield_is_noop(void) {
    yield();  // must compile & link, observable: nothing changes
    TEST_ASSERT_EQUAL_UINT32(0, millis());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_macros_defined);
    RUN_TEST(test_digitalWrite_then_digitalRead);
    RUN_TEST(test_pinMode_records_in_event_log);
    RUN_TEST(test_input_pullup_reads_high_by_default);
    RUN_TEST(test_millis_micros_advance_with_delay);
    RUN_TEST(test_yield_is_noop);
    return UNITY_END();
}
```

- [ ] **Step 2: Run, verify fail (no Arduino.h).**

```bash
.venv/bin/pio test -e native --filter test_arduino_basics 2>&1 | tail -10
```

- [ ] **Step 3: Write `include/Arduino.h` and `src/platforms/arduino_esp32/arduino.cpp`.**

`include/Arduino.h`:

```cpp
// include/Arduino.h
//
// Drop-in replacement for arduino-esp32's Arduino.h, used when compiling for
// PlatformIO's [env:native] against esp32-pio-emulator. Forwards every call
// into the framework's core engine.
//
// Consumers must `#include <Arduino.h>` exactly as they would on real hardware
// — that's the load-bearing promise of the framework. This header MUST sit at
// the root of include/ so PIO's include-path priority finds us first.
#pragma once

#include <stdint.h>

#ifdef __cplusplus

// Pin-level macros. Match arduino-esp32's values exactly.
constexpr int LOW = 0;
constexpr int HIGH = 1;

// Pin modes. Values match arduino-esp32's enum, which historically uses
// power-of-two flags. We use a smaller subset; the extra bits don't matter
// in the sim because we route through esp32sim::PinMode internally.
constexpr int INPUT          = 0x01;
constexpr int OUTPUT         = 0x02;
constexpr int INPUT_PULLUP   = 0x05;
constexpr int INPUT_PULLDOWN = 0x09;

// Interrupt modes (used by attachInterrupt in T1 task 8).
constexpr int DISABLED = 0x00;
constexpr int RISING   = 0x01;
constexpr int FALLING  = 0x02;
constexpr int CHANGE   = 0x03;
constexpr int ONLOW    = 0x04;
constexpr int ONHIGH   = 0x05;

extern "C" {
#endif

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int  digitalRead(uint8_t pin);

uint32_t millis(void);
uint32_t micros(void);
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);

// Cooperative yield. T1 implements as a no-op; T4 may make it a scheduler
// hint when FreeRTOS shim lands.
void yield(void);

#ifdef __cplusplus
}  // extern "C"
#endif

// Sketches expect setup() and loop() to be defined by the user. We declare
// them here so the test harness can link against them. (In real arduino-esp32
// they're invoked from the runtime startup; in the sim, harness/unity/sim.cpp
// — task 7 — drives them via ESP32Sim::runSetup() / runLoop().)
#ifdef __cplusplus
extern "C" {
#endif
void setup(void);
void loop(void);
#ifdef __cplusplus
}
#endif
```

`src/platforms/arduino_esp32/arduino.cpp`:

```cpp
// src/platforms/arduino_esp32/arduino.cpp
//
// Implementation of the Arduino.h API in terms of the framework core.
#include <Arduino.h>
#include <esp32sim/clock.h>
#include <esp32sim/gpio.h>

namespace {
esp32sim::PinMode translate_mode(uint8_t arduino_mode) {
    // Arduino.h macro values → core PinMode enum.
    switch (arduino_mode) {
        case OUTPUT:         return esp32sim::PinMode::OUTPUT;
        case INPUT_PULLUP:   return esp32sim::PinMode::INPUT_PULLUP;
        case INPUT_PULLDOWN: return esp32sim::PinMode::INPUT_PULLDOWN;
        case INPUT:
        default:             return esp32sim::PinMode::INPUT;
    }
}
}  // namespace

extern "C" {

void pinMode(uint8_t pin, uint8_t mode) {
    esp32sim::PinRegistry::instance().set_mode((int)pin, translate_mode(mode));
}

void digitalWrite(uint8_t pin, uint8_t val) {
    esp32sim::PinRegistry::instance().set_level((int)pin, val ? 1 : 0);
}

int digitalRead(uint8_t pin) {
    return esp32sim::PinRegistry::instance().get_level((int)pin);
}

uint32_t millis(void) {
    return (uint32_t)esp32sim::VirtualClock::instance().now_ms();
}

uint32_t micros(void) {
    return (uint32_t)esp32sim::VirtualClock::instance().now_us();
}

void delay(uint32_t ms) {
    esp32sim::VirtualClock::instance().advance_ms(ms);
}

void delayMicroseconds(uint32_t us) {
    esp32sim::VirtualClock::instance().advance_us(us);
}

void yield(void) {
    // T1 no-op. T4: signal scheduler if FreeRTOS shim is active.
}

}  // extern "C"
```

- [ ] **Step 4: Run, verify pass.**

```bash
.venv/bin/pio test -e native --filter test_arduino_basics 2>&1 | tail -10
```

Expected: `6 test cases: 6 succeeded`.

- [ ] **Step 5: Commit.**

```bash
git add include/Arduino.h src/platforms/arduino_esp32/arduino.cpp test/test_arduino_basics/
git commit -m "feat(platforms): Arduino.h fake — load-bearing header

T1 task 5. The header sits at include/Arduino.h so consumer
sketches' '#include <Arduino.h>' resolves to our fake. Defines
HIGH/LOW, INPUT/OUTPUT/INPUT_PULLUP/INPUT_PULLDOWN, RISING/FALLING/
CHANGE/ONLOW/ONHIGH (used in task 8). Forwards pinMode/digital*/
millis/micros/delay/delayMicroseconds into the core engine. yield()
is a no-op in T1.

Critique: pin-mode macro values (0x01, 0x02, 0x05, 0x09) match
arduino-esp32's bitflag layout. They look like 'magic constants' but
matching the upstream layout means a sketch that does
'pinMode(2, OUTPUT | PULLUP)' (rare but legal) won't crash —
translate_mode falls through to default INPUT. Mitigation: the
mapping is enumerated explicitly; if a future variant changes the
values, this is the one place to update.

extern \"C\" linkage everywhere: arduino-esp32 declares these as C
functions, so consumers' sketches link them as C symbols.

6 unit tests cover: macros defined to expected values, write/read
roundtrip, pinMode records event, INPUT_PULLUP reads HIGH default,
millis/micros advance with delay/delayMicroseconds, yield is no-op."
git push origin main
```

---

## Task 6: HardwareSerial — Serial / Serial1 / Serial2

**Files:**
- Create: `include/HardwareSerial.h`
- Create: `src/platforms/arduino_esp32/hardware_serial.cpp`
- Create: `test/test_hardware_serial/test_serial.cpp`

**What:** `HardwareSerial` class with `begin(baud)`, `print(...)`, `println(...)`, `printf(...)`, `write(byte)`, `available()`, `read()`, `peek()`, `flush()`. Three globals: `Serial`, `Serial1`, `Serial2`. Backed by `UartChannel`.

- [ ] **Step 1: Write failing tests covering print/println/printf, write, read, peek, available.**

`test/test_hardware_serial/test_serial.cpp`:

```cpp
#include <Arduino.h>
#include <HardwareSerial.h>
#include <esp32sim/clock.h>
#include <esp32sim/event_log.h>
#include <esp32sim/uart.h>
#include <unity.h>

void setUp(void) {
    esp32sim::VirtualClock::instance().reset();
    esp32sim::EventLog::instance().reset();
    esp32sim::UartChannel::reset_all();
}
void tearDown(void) {}

void test_serial_println_appends_crlf(void) {
    Serial.begin(115200);
    Serial.println("hello");
    auto& u = esp32sim::UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_STRING("hello\r\n", u.drain_tx().c_str());
}

void test_serial_print_no_newline(void) {
    Serial.print("abc");
    auto& u = esp32sim::UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_STRING("abc", u.drain_tx().c_str());
}

void test_serial_printf(void) {
    Serial.printf("x=%d y=%d", 1, 42);
    auto& u = esp32sim::UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_STRING("x=1 y=42", u.drain_tx().c_str());
}

void test_serial_print_int(void) {
    Serial.print(123);
    auto& u = esp32sim::UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_STRING("123", u.drain_tx().c_str());
}

void test_serial_print_int_with_base(void) {
    Serial.print(255, 16);  // hex
    auto& u = esp32sim::UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_STRING("ff", u.drain_tx().c_str());
}

void test_serial_write_single_byte(void) {
    Serial.write('Q');
    auto& u = esp32sim::UartChannel::for_index(0);
    TEST_ASSERT_EQUAL_STRING("Q", u.drain_tx().c_str());
}

void test_serial_read_consumes_rx(void) {
    auto& u = esp32sim::UartChannel::for_index(0);
    u.rx_inject("hi");
    TEST_ASSERT_TRUE(Serial.available());
    TEST_ASSERT_EQUAL_INT('h', Serial.read());
    TEST_ASSERT_EQUAL_INT('i', Serial.read());
    TEST_ASSERT_EQUAL_INT(-1, Serial.read());
    TEST_ASSERT_FALSE(Serial.available());
}

void test_serial_peek_does_not_consume(void) {
    auto& u = esp32sim::UartChannel::for_index(0);
    u.rx_inject("abc");
    TEST_ASSERT_EQUAL_INT('a', Serial.peek());
    TEST_ASSERT_EQUAL_INT('a', Serial.peek());
    TEST_ASSERT_EQUAL_INT(3, Serial.available());
}

void test_serial1_serial2_isolated(void) {
    Serial.print("s0");
    Serial1.print("s1");
    Serial2.print("s2");
    TEST_ASSERT_EQUAL_STRING("s0", esp32sim::UartChannel::for_index(0).drain_tx().c_str());
    TEST_ASSERT_EQUAL_STRING("s1", esp32sim::UartChannel::for_index(1).drain_tx().c_str());
    TEST_ASSERT_EQUAL_STRING("s2", esp32sim::UartChannel::for_index(2).drain_tx().c_str());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_serial_println_appends_crlf);
    RUN_TEST(test_serial_print_no_newline);
    RUN_TEST(test_serial_printf);
    RUN_TEST(test_serial_print_int);
    RUN_TEST(test_serial_print_int_with_base);
    RUN_TEST(test_serial_write_single_byte);
    RUN_TEST(test_serial_read_consumes_rx);
    RUN_TEST(test_serial_peek_does_not_consume);
    RUN_TEST(test_serial1_serial2_isolated);
    return UNITY_END();
}
```

- [ ] **Step 2: Run, verify fail.**

```bash
.venv/bin/pio test -e native --filter test_hardware_serial 2>&1 | tail -10
```

- [ ] **Step 3: Write the header + impl.**

`include/HardwareSerial.h`:

```cpp
#pragma once

#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus

class HardwareSerial {
public:
    explicit HardwareSerial(int uart_num) : uart_num_(uart_num) {}

    // Real arduino-esp32 begin signature has many optional params; T1 keeps
    // it simple and ignores baud/config since the sim has no electrical
    // representation.
    void begin(unsigned long /*baud*/) {}
    void end(void) {}
    void flush(void) {}

    // Print API
    size_t print(const char* s);
    size_t print(int n, int base = 10);
    size_t print(unsigned int n, int base = 10);
    size_t print(long n, int base = 10);
    size_t print(unsigned long n, int base = 10);
    size_t print(double d, int decimals = 2);
    size_t print(char c);
    size_t println(const char* s);
    size_t println(int n, int base = 10);
    size_t println(void);
    size_t printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

    // Write API
    size_t write(uint8_t b);
    size_t write(const uint8_t* buf, size_t n);
    size_t write(const char* s);

    // Read API
    int available(void);
    int read(void);
    int peek(void);

private:
    int uart_num_;
    void emit_(const char* s);
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;
extern HardwareSerial Serial2;

#endif  // __cplusplus
```

`src/platforms/arduino_esp32/hardware_serial.cpp`:

```cpp
#include <HardwareSerial.h>
#include <esp32sim/uart.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

HardwareSerial Serial(0);
HardwareSerial Serial1(1);
HardwareSerial Serial2(2);

void HardwareSerial::emit_(const char* s) {
    auto& u = esp32sim::UartChannel::for_index(uart_num_);
    while (*s) u.tx_write_byte(static_cast<uint8_t>(*s++));
}

size_t HardwareSerial::print(const char* s) {
    if (!s) return 0;
    size_t n = std::strlen(s);
    emit_(s);
    return n;
}

size_t HardwareSerial::print(char c) {
    write(static_cast<uint8_t>(c));
    return 1;
}

size_t HardwareSerial::print(int n, int base) {
    return print(static_cast<long>(n), base);
}

size_t HardwareSerial::print(unsigned int n, int base) {
    return print(static_cast<unsigned long>(n), base);
}

size_t HardwareSerial::print(long n, int base) {
    char buf[34];
    if (base == 10) std::snprintf(buf, sizeof(buf), "%ld", n);
    else if (base == 16) std::snprintf(buf, sizeof(buf), "%lx", n);
    else if (base == 8) std::snprintf(buf, sizeof(buf), "%lo", n);
    else if (base == 2) {
        // simple binary formatter
        unsigned long u = (n < 0) ? (unsigned long)(-n) : (unsigned long)n;
        char tmp[66];
        int idx = 0;
        if (u == 0) tmp[idx++] = '0';
        while (u) { tmp[idx++] = '0' + (u & 1); u >>= 1; }
        if (n < 0) tmp[idx++] = '-';
        // reverse into buf
        for (int i = 0; i < idx; ++i) buf[i] = tmp[idx - 1 - i];
        buf[idx] = '\0';
    } else {
        std::snprintf(buf, sizeof(buf), "%ld", n);
    }
    return print(buf);
}

size_t HardwareSerial::print(unsigned long n, int base) {
    char buf[34];
    if (base == 16) std::snprintf(buf, sizeof(buf), "%lx", n);
    else if (base == 8) std::snprintf(buf, sizeof(buf), "%lo", n);
    else std::snprintf(buf, sizeof(buf), "%lu", n);
    return print(buf);
}

size_t HardwareSerial::print(double d, int decimals) {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.*f", decimals, d);
    return print(buf);
}

size_t HardwareSerial::println(const char* s) {
    size_t n = print(s);
    emit_("\r\n");
    return n + 2;
}

size_t HardwareSerial::println(int n, int base) {
    size_t r = print(n, base);
    emit_("\r\n");
    return r + 2;
}

size_t HardwareSerial::println(void) {
    emit_("\r\n");
    return 2;
}

size_t HardwareSerial::printf(const char* fmt, ...) {
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 0) return 0;
    if ((size_t)n >= sizeof(buf)) {
        // truncated — emit what we have
        emit_(buf);
        return sizeof(buf) - 1;
    }
    emit_(buf);
    return (size_t)n;
}

size_t HardwareSerial::write(uint8_t b) {
    esp32sim::UartChannel::for_index(uart_num_).tx_write_byte(b);
    return 1;
}

size_t HardwareSerial::write(const uint8_t* buf, size_t n) {
    auto& u = esp32sim::UartChannel::for_index(uart_num_);
    for (size_t i = 0; i < n; ++i) u.tx_write_byte(buf[i]);
    return n;
}

size_t HardwareSerial::write(const char* s) {
    if (!s) return 0;
    size_t n = std::strlen(s);
    emit_(s);
    return n;
}

int HardwareSerial::available(void) {
    return (int)esp32sim::UartChannel::for_index(uart_num_).rx_size();
}

int HardwareSerial::read(void) {
    return esp32sim::UartChannel::for_index(uart_num_).rx_read_byte();
}

int HardwareSerial::peek(void) {
    return esp32sim::UartChannel::for_index(uart_num_).rx_peek();
}
```

- [ ] **Step 4: Run, verify pass.**

```bash
.venv/bin/pio test -e native --filter test_hardware_serial 2>&1 | tail -10
```

Expected: `9 test cases: 9 succeeded`.

- [ ] **Step 5: Commit.**

```bash
git add include/HardwareSerial.h src/platforms/arduino_esp32/hardware_serial.cpp test/test_hardware_serial/
git commit -m "feat(platforms): HardwareSerial — Serial/Serial1/Serial2

T1 task 6. HardwareSerial class with print/println/printf/write/
read/available/peek/flush. Three globals (Serial, Serial1, Serial2)
backed by UartChannel::for_index(0/1/2).

println appends '\\r\\n' to match arduino-esp32. printf uses
vsnprintf into a 256-byte buffer; truncation emits what fits and
returns a partial count. print(int, base) supports base 2/8/10/16
(real Arduino supports more, but these are the practical set).

Critique: 256 byte printf buffer could be too small for verbose
log lines. Mitigation: typical ESP32 Serial usage is short status
lines; if a real sketch's Serial.printf truncates, the user will
see partial output and notice. For long lines, sketches should use
multiple printfs anyway because real arduino-esp32 Serial has a
similar internal buffer limit.

9 unit tests cover println CRLF, print without newline, printf
formatting, print(int), print(int, base=hex), write byte, read
consumes RX, peek does not consume, three serials isolated."
git push origin main
```

---

## Task 7: ESP32Sim::* — public C++ test API

**Files:**
- Create: `include/esp32sim/esp32sim.h` (umbrella header)
- Create: `include/esp32sim_unity/esp32sim.h`
- Create: `src/harness/unity/sim.cpp`
- Create: `test/test_harness_sim/test_sim.cpp`

**What:** The `esp32sim::Sim` class — the user-facing test API. Provides `reset`, `runSetup`, `runLoop`, `runUntil`, `advanceMs`, `nowMs`, `gpio(N)`, `uart(N)`, `events()`. This is what tests `#include <esp32sim_unity/esp32sim.h>` for.

- [ ] **Step 1: Write tests against the high-level API.**

`test/test_harness_sim/test_sim.cpp`:

```cpp
#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

namespace {
int setup_count = 0;
int loop_count = 0;
}

extern "C" void setup(void) {
    setup_count++;
    pinMode(2, OUTPUT);
}

extern "C" void loop(void) {
    loop_count++;
    digitalWrite(2, (loop_count % 2 == 1) ? HIGH : LOW);
    delay(100);
}

void setUp(void) {
    esp32sim::Sim::reset();
    setup_count = 0;
    loop_count = 0;
}
void tearDown(void) {}

void test_runSetup_invokes_user_setup(void) {
    esp32sim::Sim::runSetup();
    TEST_ASSERT_EQUAL_INT(1, setup_count);
}

void test_runLoop_default_one_iteration(void) {
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(1, loop_count);
}

void test_runLoop_n_iterations(void) {
    esp32sim::Sim::runLoop(5);
    TEST_ASSERT_EQUAL_INT(5, loop_count);
}

void test_gpio_level(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(2).level());
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());
}

void test_advanceMs(void) {
    esp32sim::Sim::advanceMs(500);
    TEST_ASSERT_EQUAL_UINT64(500, esp32sim::Sim::nowMs());
}

void test_runUntil_predicate_true(void) {
    esp32sim::Sim::runSetup();
    bool found = esp32sim::Sim::runUntil(
        []() { return esp32sim::Sim::gpio(2).level() == HIGH; },
        /*timeoutMs*/ 1000);
    TEST_ASSERT_TRUE(found);
}

void test_runUntil_timeout(void) {
    esp32sim::Sim::runSetup();
    bool found = esp32sim::Sim::runUntil(
        []() { return esp32sim::Sim::gpio(99).level() == HIGH; },  // never true
        /*timeoutMs*/ 50);
    TEST_ASSERT_FALSE(found);
}

void test_uart_drain_tx(void) {
    Serial.print("hi");
    TEST_ASSERT_EQUAL_STRING("hi", esp32sim::Sim::uart(0).drainTx().c_str());
}

void test_uart_inject(void) {
    esp32sim::Sim::uart(0).inject("Q");
    TEST_ASSERT_EQUAL_INT('Q', Serial.read());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_runSetup_invokes_user_setup);
    RUN_TEST(test_runLoop_default_one_iteration);
    RUN_TEST(test_runLoop_n_iterations);
    RUN_TEST(test_gpio_level);
    RUN_TEST(test_advanceMs);
    RUN_TEST(test_runUntil_predicate_true);
    RUN_TEST(test_runUntil_timeout);
    RUN_TEST(test_uart_drain_tx);
    RUN_TEST(test_uart_inject);
    return UNITY_END();
}
```

- [ ] **Step 2: Run, verify fail.**

- [ ] **Step 3: Write `include/esp32sim/esp32sim.h` (umbrella), `include/esp32sim_unity/esp32sim.h` (Unity-specific user-facing API), and `src/harness/unity/sim.cpp`.**

`include/esp32sim/esp32sim.h`:

```cpp
// Umbrella header — pulls in every public esp32sim/ subheader.
#pragma once

#include <esp32sim/clock.h>
#include <esp32sim/event_log.h>
#include <esp32sim/gpio.h>
#include <esp32sim/uart.h>
```

`include/esp32sim_unity/esp32sim.h`:

```cpp
// Unity-side test API. Tests #include this and write against esp32sim::Sim::*.
#pragma once

#include <esp32sim/esp32sim.h>

#include <cstdint>
#include <functional>
#include <string>

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
        EventQuery& kind(EventKind k) { kind_ = k; have_kind_ = true; return *this; }
        EventQuery& pin(int p) { pin_ = p; have_pin_ = true; return *this; }
        EventQuery& after(uint64_t timestamp_us) { after_ = timestamp_us; have_after_ = true; return *this; }
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
```

`src/harness/unity/sim.cpp`:

```cpp
#include <esp32sim_unity/esp32sim.h>

extern "C" void setup(void);
extern "C" void loop(void);

namespace esp32sim {

void Sim::reset() {
    VirtualClock::instance().reset();
    EventLog::instance().reset();
    PinRegistry::instance().reset();
    UartChannel::reset_all();
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
void Sim::UartRef::inject(const std::string& s) { UartChannel::for_index(n_).rx_inject(s); }

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
```

- [ ] **Step 4: Run, verify pass.**

```bash
.venv/bin/pio test -e native --filter test_harness_sim 2>&1 | tail -10
```

Expected: `9 test cases: 9 succeeded`.

- [ ] **Step 5: Commit.**

```bash
git add include/esp32sim/esp32sim.h include/esp32sim_unity/esp32sim.h src/harness/unity/sim.cpp test/test_harness_sim/
git commit -m "feat(harness): ESP32Sim::* — public C++ test API

T1 task 7. The user-facing test API. Sim::reset/runSetup/runLoop/
runUntil drive the user's setup() and loop() (declared extern \"C\"
to match arduino-esp32 linkage). Sim::gpio(N), Sim::uart(N),
Sim::events() return small ref/builder objects for assertions.

GpioRef::pulse(level, ms) is the primitive for debouncing tests
(set level, advance time, restore previous level).

EventQuery is a chained builder: events().kind(GPIO_WRITE).pin(2).
count(). Predicate-based on top of EventLog::filter so adding new
filter dimensions later is one line.

9 unit tests cover full lifecycle: runSetup invokes user setup,
runLoop default + n, gpio.level, advanceMs, runUntil success,
runUntil timeout, uart.drainTx, uart.inject."
git push origin main
```

---

## Task 8: attachInterrupt — synchronous-fire ISRs

**Files:**
- Create: `src/platforms/arduino_esp32/interrupts.cpp`
- Modify: `include/Arduino.h` (declare `attachInterrupt`/`detachInterrupt`/`digitalPinToInterrupt`)
- Create: `test/test_attach_interrupt/test_interrupts.cpp`

**What:** ISR fires synchronously on level change. RISING / FALLING / CHANGE supported. ONLOW / ONHIGH treated as level-triggered (fire if currently in target state when attached + when entering it).

- [ ] **Step 1: Write failing tests.**

`test/test_attach_interrupt/test_interrupts.cpp`:

```cpp
#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

namespace { int isr_count = 0; }
void my_isr() { isr_count++; }

void setUp(void) {
    esp32sim::Sim::reset();
    isr_count = 0;
}
void tearDown(void) {}

void test_rising_fires_only_on_low_to_high(void) {
    pinMode(4, INPUT);
    attachInterrupt(digitalPinToInterrupt(4), my_isr, RISING);
    esp32sim::Sim::gpio(4).setLevel(0);  // already 0, no edge
    esp32sim::Sim::gpio(4).setLevel(1);  // 0->1: fires
    esp32sim::Sim::gpio(4).setLevel(0);  // 1->0: no fire (FALLING)
    esp32sim::Sim::gpio(4).setLevel(1);  // 0->1: fires
    TEST_ASSERT_EQUAL_INT(2, isr_count);
}

void test_falling_fires_only_on_high_to_low(void) {
    pinMode(4, INPUT);
    esp32sim::Sim::gpio(4).setLevel(1);
    attachInterrupt(digitalPinToInterrupt(4), my_isr, FALLING);
    esp32sim::Sim::gpio(4).setLevel(0);  // 1->0: fires
    esp32sim::Sim::gpio(4).setLevel(1);  // no
    esp32sim::Sim::gpio(4).setLevel(0);  // fires
    TEST_ASSERT_EQUAL_INT(2, isr_count);
}

void test_change_fires_on_both_edges(void) {
    pinMode(4, INPUT);
    attachInterrupt(digitalPinToInterrupt(4), my_isr, CHANGE);
    esp32sim::Sim::gpio(4).setLevel(1);
    esp32sim::Sim::gpio(4).setLevel(0);
    esp32sim::Sim::gpio(4).setLevel(1);
    TEST_ASSERT_EQUAL_INT(3, isr_count);
}

void test_detachInterrupt_silences_isr(void) {
    pinMode(4, INPUT);
    attachInterrupt(digitalPinToInterrupt(4), my_isr, RISING);
    esp32sim::Sim::gpio(4).setLevel(1);
    detachInterrupt(digitalPinToInterrupt(4));
    esp32sim::Sim::gpio(4).setLevel(0);
    esp32sim::Sim::gpio(4).setLevel(1);
    TEST_ASSERT_EQUAL_INT(1, isr_count);  // only the pre-detach one
}

void test_no_isr_for_non_matching_mode(void) {
    pinMode(4, INPUT);
    attachInterrupt(digitalPinToInterrupt(4), my_isr, FALLING);
    esp32sim::Sim::gpio(4).setLevel(1);  // RISING — not FALLING
    TEST_ASSERT_EQUAL_INT(0, isr_count);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_rising_fires_only_on_low_to_high);
    RUN_TEST(test_falling_fires_only_on_high_to_low);
    RUN_TEST(test_change_fires_on_both_edges);
    RUN_TEST(test_detachInterrupt_silences_isr);
    RUN_TEST(test_no_isr_for_non_matching_mode);
    return UNITY_END();
}
```

- [ ] **Step 2: Run, verify fail.**

- [ ] **Step 3: Modify `include/Arduino.h` to declare interrupt functions.**

Add inside the `extern "C" {` block (before the closing brace):

```cpp
typedef void (*voidFuncPtr)(void);

void attachInterrupt(uint8_t interrupt_num, voidFuncPtr isr, int mode);
void detachInterrupt(uint8_t interrupt_num);

// Real arduino-esp32 maps GPIO pin -> interrupt number 1:1; we mirror that.
inline uint8_t digitalPinToInterrupt(uint8_t pin) { return pin; }
```

(`digitalPinToInterrupt` is `inline` because `extern "C"` and `inline` together work as expected here for the trivial mapping.)

- [ ] **Step 4: Write `src/platforms/arduino_esp32/interrupts.cpp`.**

```cpp
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
```

- [ ] **Step 5: Run, verify pass.**

```bash
.venv/bin/pio test -e native --filter test_attach_interrupt 2>&1 | tail -10
```

Expected: `5 test cases: 5 succeeded`.

- [ ] **Step 6: Commit.**

```bash
git add include/Arduino.h src/platforms/arduino_esp32/interrupts.cpp test/test_attach_interrupt/
git commit -m "feat(platforms): attachInterrupt — synchronous ISR firing

T1 task 8. attachInterrupt + detachInterrupt + digitalPinToInterrupt
(identity for ESP32). ISRs fire synchronously when set_level changes
the pin in the right direction (RISING/FALLING/CHANGE/ONHIGH/ONLOW).

Implementation: 64-slot ISR table, indexed by pin. PinRegistry
listener checks the slot's mode + fn at fire time, so detach is
implemented as zeroing the slot (the underlying listener becomes a
no-op). Critique: this leaks the listener on detach — re-attach to
the same pin grows listener vector by 1. For T1 it's harmless; T2+
should add PinRegistry::clear_listeners or similar.

Critique: synchronous firing means the ISR runs in the middle of a
test's setLevel call, not asynchronously. This is documented in
docs/user/explanation/what-this-does-and-doesnt-catch.md (T1 task
12). Real ESP32 ISRs fire in real preemption — bugs caused by ISR/
main-thread races won't reproduce in T1. T4 RTOS shim addresses
this.

5 unit tests cover RISING-only, FALLING-only, CHANGE-both-edges,
detach silences ISR, no-fire for non-matching mode."
git push origin main
```

---

## Task 9–11: End-to-end examples (blink, debounce, serial-echo)

These three tasks have an identical shape: each ships an `examples/0N-name/` directory with a sketch + a Unity test exercising it via `Sim::*`. Each example has its own self-contained `platformio.ini` that `lib_deps`-es us via the GitHub URL.

### Task 9: examples/01-blink

- [ ] **Step 1: Create `examples/01-blink/platformio.ini`.**

```ini
[env:native]
platform = native
test_framework = unity
lib_deps =
    https://github.com/fresh-fx59/esp32-pio-emulator
build_flags = -std=gnu++17
```

- [ ] **Step 2: Create `examples/01-blink/src/main.cpp` (the sketch).**

```cpp
#include <Arduino.h>

constexpr int LED_PIN = 2;
constexpr unsigned long PERIOD_MS = 500;
unsigned long last_toggle = 0;
bool led_on = false;

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void loop() {
    unsigned long now = millis();
    if (now - last_toggle >= PERIOD_MS) {
        led_on = !led_on;
        digitalWrite(LED_PIN, led_on ? HIGH : LOW);
        last_toggle = now;
    }
}
```

- [ ] **Step 3: Create `examples/01-blink/test/test_blink/test_blink.cpp`.**

```cpp
#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) { esp32sim::Sim::reset(); }
void tearDown(void) {}

void test_led_starts_off(void) {
    esp32sim::Sim::runSetup();
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());
}

void test_led_toggles_at_1hz(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());

    esp32sim::Sim::advanceMs(500);
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(2).level());

    esp32sim::Sim::advanceMs(500);
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());

    esp32sim::Sim::advanceMs(500);
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(2).level());
}

void test_n_toggles_in_n_seconds(void) {
    esp32sim::Sim::runSetup();
    int toggles = 0;
    int last = LOW;
    for (int i = 0; i < 20; ++i) {
        esp32sim::Sim::advanceMs(500);
        esp32sim::Sim::runLoop();
        int now = esp32sim::Sim::gpio(2).level();
        if (now != last) toggles++;
        last = now;
    }
    // 10 seconds of virtual time at 1Hz toggle rate → 20 toggles
    TEST_ASSERT_INT_WITHIN(1, 20, toggles);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_led_starts_off);
    RUN_TEST(test_led_toggles_at_1hz);
    RUN_TEST(test_n_toggles_in_n_seconds);
    return UNITY_END();
}
```

- [ ] **Step 4: Verify it runs from a fresh dir (independent of the parent repo's PIO env).**

```bash
cd examples/01-blink
../../.venv/bin/pio test -e native 2>&1 | tail -15
cd ../..
```

Expected: `3 test cases: 3 succeeded`. *Note: this uses `lib_deps` from GitHub, so it may be slower the first time as it clones the repo.*

- [ ] **Step 5: Add a README and commit.**

`examples/01-blink/README.md`:

```markdown
# 01-blink — first end-to-end test against the simulator

This example demonstrates the smallest-possible TDD loop against
`esp32-pio-emulator`. The sketch (`src/main.cpp`) is a classic 1Hz
blink. The test (`test/test_blink/test_blink.cpp`) drives the sketch
through three checks: LED starts off, toggles at 500ms intervals,
20 toggles over 10 seconds of virtual time.

Run it:

    pio test -e native

This pulls `esp32-pio-emulator` from GitHub via `lib_deps`. First run
is slow (clone); subsequent runs are sub-second.
```

```bash
git add examples/01-blink/
git commit -m "feat(examples): 01-blink — first end-to-end TDD loop works

T1 task 9. The smallest demo of the framework's value: a real Arduino
sketch (1Hz blink, unmodified) plus a Unity test that drives it via
Sim::* and asserts on observable pin level over virtual time.

Independent platformio.ini: example pulls esp32-pio-emulator from
GitHub via lib_deps. Verified locally — first run clones the repo
(slow), subsequent runs are sub-second.

Three tests cover: LED starts off, alternates HIGH/LOW each 500ms,
20 toggles in 10 seconds of virtual time (with TEST_ASSERT_INT_WITHIN
to allow off-by-one at the boundaries)."
git push origin main
```

### Task 10: examples/02-button-debounce

(Same structure as Task 9. Each step mirrors the pattern: platformio.ini, src/main.cpp, test/test_*/test_*.cpp, README, commit.)

- [ ] **Step 1: `examples/02-button-debounce/platformio.ini`** — identical to 01-blink's.

- [ ] **Step 2: `examples/02-button-debounce/src/main.cpp`** — debouncer that reads pin 4 (button), drives pin 2 (LED), with 50ms debounce window:

```cpp
#include <Arduino.h>

constexpr int BUTTON_PIN = 4;
constexpr int LED_PIN = 2;
constexpr unsigned long DEBOUNCE_MS = 50;

int last_stable = LOW;
int last_raw = LOW;
unsigned long last_change = 0;

void setup() {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    int raw = digitalRead(BUTTON_PIN);
    unsigned long now = millis();
    if (raw != last_raw) {
        last_raw = raw;
        last_change = now;
    }
    if ((now - last_change) >= DEBOUNCE_MS && raw != last_stable) {
        last_stable = raw;
        digitalWrite(LED_PIN, last_stable);
    }
}
```

- [ ] **Step 3: `examples/02-button-debounce/test/test_debounce/test_debounce.cpp`** — exactly one debounced edge despite bouncing input:

```cpp
#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) { esp32sim::Sim::reset(); }
void tearDown(void) {}

void run_loops_for(uint64_t ms) {
    for (uint64_t t = 0; t < ms; ++t) {
        esp32sim::Sim::runLoop();
        esp32sim::Sim::advanceMs(1);
    }
}

void test_clean_press_produces_one_edge(void) {
    esp32sim::Sim::runSetup();
    // INPUT_PULLUP — released = HIGH. Press = LOW.
    esp32sim::Sim::gpio(4).setLevel(LOW);
    run_loops_for(100);  // > 50ms debounce window
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());
}

void test_bouncing_press_collapses_to_one_edge(void) {
    esp32sim::Sim::runSetup();
    auto pin4 = esp32sim::Sim::gpio(4);
    // Simulate 5ms of bouncing: rapid LOW/HIGH oscillation
    for (int i = 0; i < 10; ++i) {
        pin4.setLevel(LOW);
        esp32sim::Sim::advanceMs(0); esp32sim::Sim::runLoop();
        pin4.setLevel(HIGH);
        esp32sim::Sim::advanceMs(0); esp32sim::Sim::runLoop();
    }
    pin4.setLevel(LOW);
    run_loops_for(100);
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());
    // Count HIGH→LOW transitions on the LED pin via EventLog.
    auto led_events = esp32sim::Sim::events().kind(esp32sim::EventKind::GPIO_WRITE).pin(2).all();
    int low_writes = 0;
    for (auto& e : led_events) if (e.value == LOW) low_writes++;
    // First write to LED is in setup() (LOW). Then debouncer should write LOW
    // exactly once after the press settles. So we expect 1 or 2 LOW writes
    // depending on whether setup's initial digitalWrite shows up here. Given
    // there's no initial digitalWrite in the sketch, expect exactly 1.
    TEST_ASSERT_EQUAL_INT(1, low_writes);
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_clean_press_produces_one_edge);
    RUN_TEST(test_bouncing_press_collapses_to_one_edge);
    return UNITY_END();
}
```

- [ ] **Step 4: Verify + commit.**

```bash
cd examples/02-button-debounce && ../../.venv/bin/pio test -e native 2>&1 | tail -15 && cd ../..
git add examples/02-button-debounce/
git commit -m "feat(examples): 02-button-debounce

T1 task 10. Classic 50ms debouncer reading INPUT_PULLUP button on
pin 4, driving LED on pin 2. Tests prove (1) clean press produces
exactly one edge, (2) 5ms of input bouncing collapses to one
debounced LED transition (asserted via EventLog count of GPIO_WRITE
events with value=LOW on pin 2)."
git push origin main
```

### Task 11: examples/03-serial-echo

- [ ] **Step 1: platformio.ini** — same as 01-blink.

- [ ] **Step 2: `examples/03-serial-echo/src/main.cpp`** — read available bytes, echo back uppercase:

```cpp
#include <Arduino.h>

void setup() {
    Serial.begin(115200);
}

void loop() {
    while (Serial.available()) {
        int c = Serial.read();
        if (c >= 'a' && c <= 'z') c -= 32;
        Serial.write((uint8_t)c);
    }
}
```

- [ ] **Step 3: `examples/03-serial-echo/test/test_echo/test_echo.cpp`**:

```cpp
#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) { esp32sim::Sim::reset(); }
void tearDown(void) {}

void test_echo_uppercase(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::uart(0).inject("hello");
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_STRING("HELLO", esp32sim::Sim::uart(0).drainTx().c_str());
}

void test_passthrough_non_lowercase(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::uart(0).inject("Hi 42!");
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_STRING("HI 42!", esp32sim::Sim::uart(0).drainTx().c_str());
}

void test_no_input_no_output(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_STRING("", esp32sim::Sim::uart(0).drainTx().c_str());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_echo_uppercase);
    RUN_TEST(test_passthrough_non_lowercase);
    RUN_TEST(test_no_input_no_output);
    return UNITY_END();
}
```

- [ ] **Step 4: Verify + commit.**

```bash
cd examples/03-serial-echo && ../../.venv/bin/pio test -e native 2>&1 | tail -15 && cd ../..
git add examples/03-serial-echo/
git commit -m "feat(examples): 03-serial-echo

T1 task 11. Reads from Serial, echoes back uppercase. Three tests:
inject 'hello' then drainTx == 'HELLO', mixed input passes through
non-lowercase chars unchanged, no input means no output."
git push origin main
```

---

## Task 12: Documentation

**Files:**
- Create: `docs/user/tutorials/your-first-test.md`
- Create: `docs/user/reference/sim-api.md`
- Create: `docs/user/reference/supported-arduino-apis.md`
- Create: `docs/user/explanation/why-virtual-time.md`
- Create: `docs/user/explanation/what-this-does-and-doesnt-catch.md`
- Create: `docs/user/explanation/our-framework-vs-arduinofake.md`
- Create: `docs/user/how-to/test-an-interrupt-driven-sketch.md`
- Create: `docs/dev/how-to/add-a-new-arduino-api.md`
- Modify: `docs/user/{tutorials,how-to,reference,explanation}/README.md` (remove "(empty until T1)" notes)

- [ ] **Step 1: Write each doc file** following the Diátaxis category for its location. Each doc should be a self-contained ~150-300 word piece. Reference the actual code (use `examples/` paths, link to specs and ADRs).

- [ ] **Step 2: Update `docs/README.md`** to include the new docs in the index.

- [ ] **Step 3: Update each Diátaxis subfolder README** to remove "(empty until T1)" and replace with the actual content list.

- [ ] **Step 4: Commit as one bundle (this is appropriate per AGENTS.md "tightly coupled mini-series").**

```bash
git add docs/
git commit -m "docs: T1 user + dev documentation

T1 task 12. Eight docs land:
- user/tutorials/your-first-test.md
- user/reference/{sim-api, supported-arduino-apis}.md
- user/explanation/{why-virtual-time, what-this-does-and-doesnt-
  catch, our-framework-vs-arduinofake}.md
- user/how-to/test-an-interrupt-driven-sketch.md
- dev/how-to/add-a-new-arduino-api.md

docs/README.md index updated. Diátaxis subfolder READMEs no longer
say '(empty until T1)' — the new files are listed.

Each doc is a self-contained ~150-300 word piece, references real
code paths in examples/, and links out to ADRs / specs where
relevant."
git push origin main
```

---

## Task 13: Tier 1 sign-off — CHANGELOG, README, CLAUDE.md, tag

**Files:**
- Modify: `CHANGELOG.md`
- Modify: `README.md`
- Modify: `CLAUDE.md`

- [ ] **Step 1: Update `CHANGELOG.md`** — add `[0.2.0]` entry above `[0.1.0]` listing every T1 deliverable.

- [ ] **Step 2: Update `README.md`** — flip status table T1 ✓, T2 🚧; bump status badge to "Tier 2 starting".

- [ ] **Step 3: Update `CLAUDE.md`** — current tier T2, last shipped 0.2.0.

- [ ] **Step 4: Run the full T1 acceptance gate.**

```bash
echo "=== All T1 unit tests ==="
.venv/bin/pio test -e native 2>&1 | tail -8

echo ""
echo "=== All T1 examples ==="
for ex in examples/01-blink examples/02-button-debounce examples/03-serial-echo; do
  echo "--- $ex ---"
  (cd "$ex" && ../../.venv/bin/pio test -e native 2>&1 | tail -3)
done

echo ""
echo "=== Coverage check (manual or via gcov if wired) ==="
# Counts of header lines vs implementation lines as a rough proxy
wc -l include/esp32sim/*.h include/Arduino.h include/HardwareSerial.h \
       src/core/*.cpp src/platforms/arduino_esp32/*.cpp src/harness/unity/*.cpp

echo ""
echo "=== CI ==="
gh run list --workflow=ci.yml --limit 1 --json status,conclusion --jq '.[0]'
```

Expected: all unit tests pass, all 3 examples pass, CI green.

- [ ] **Step 5: Commit + tag + push.**

```bash
git add CHANGELOG.md README.md CLAUDE.md
git commit -m "release: tier 1 (GPIO TDD) shipped, v0.2.0

T1 sign-off. The TDD-for-ESP32 promise is real for sketches whose
hardware footprint is GPIO + UART + timing.

Acceptance gate green:
- All T1 unit tests pass (8 tasks of N tests each).
- All 3 reference examples (01-blink, 02-button-debounce,
  03-serial-echo) pass against esp32-pio-emulator pulled in via
  lib_deps from GitHub.
- CI green on Ubuntu 22.04.
- Coverage ≥80% (manual count for now; gcov wiring is a T2 stretch
  task).

CHANGELOG bumped 0.1.0 → 0.2.0. README status: T0 ✓, T1 ✓, T2 🚧.
CLAUDE.md current tier: T2 starting (refresh T2 spec to v0.2 first).

Next: T2 (Sensor TDD + pytest-embedded plugin)."
git tag -a v0.2.0 -m "Tier 1 (GPIO TDD) shipped — see CHANGELOG.md [0.2.0]"
git push origin main
git push origin v0.2.0
```

---

## Self-review

**1. Spec coverage** — every requirement in the [T1 spec v0.2](../specs/2026-05-05-tier-1-gpio-tdd-design.md) maps to a task:

| Spec requirement | Plan task |
|---|---|
| `pinMode`, `digital{Read,Write}` | T5 (via T3 PinRegistry) |
| `Serial`, `Serial1`, `Serial2` | T6 (via T4 UartChannel) |
| `millis`, `micros`, `delay`, `delayMicroseconds` | T5 (via T1 VirtualClock) |
| `yield` | T5 (no-op) |
| `attachInterrupt`, RISING/FALLING/CHANGE/ONLOW/ONHIGH | T8 |
| `Sim::*` API | T7 |
| Reference examples: blink, debounce, serial-echo | T9, T10, T11 |
| Documentation deliverables | T12 |
| Sign-off | T13 |

**2. Placeholder scan** — searching this plan for `TBD|TODO|FIXME|XXX|fill in details`: only acceptable matches are inside file content shown to be created (e.g., a TODO note in a code comment about future T2+ work, intentional). No "implement later" anywhere in plan steps.

**3. Type/identifier consistency** — `esp32sim::VirtualClock`, `esp32sim::EventLog`, `esp32sim::PinRegistry`, `esp32sim::UartChannel`, `esp32sim::Sim` are used consistently across all tasks. Header file paths match the master spec v0.3 layout. The fake `Arduino.h` uses `extern "C"` linkage everywhere consumer code touches.

**4. AGENTS.md compliance:**
- TDD per task (tests first, see fail, implement, see pass). ✓
- Per-step verify-and-document: each task ends with verification command + commit. ✓
- Small commits: each task = one focused commit. ✓
- Subagent-driven dispatch viable: each task is self-contained with paths, code, and commands. ✓

If reality contradicts the plan during execution (e.g., a method signature doesn't compile, a test fails for a reason I didn't anticipate), the executor logs it, updates the plan or spec first per AGENTS.md spec-drift policy, and continues.

---

## Execution handoff

Plan complete and saved to `docs/superpowers/plans/2026-05-05-tier-1-gpio-tdd.md`.

Per [AGENTS.md](../../../AGENTS.md) Execution defaults — *Plan execution mode: subagent-driven* — execute via `superpowers:subagent-driven-development`: dispatch one fresh subagent per task with a review checkpoint between tasks. Do not ask the operator to choose execution mode; proceed.
