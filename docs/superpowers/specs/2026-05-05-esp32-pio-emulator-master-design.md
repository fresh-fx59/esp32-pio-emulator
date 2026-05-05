# esp32-pio-emulator — master design spec

| | |
|---|---|
| **Status** | Draft v0.1 — pending user review |
| **Date** | 2026-05-05 |
| **Author** | fresh-fx59 (with Claude Opus 4.7) |
| **Supersedes** | — |
| **Related** | [Tier 0](2026-05-05-tier-0-skeleton-design.md) · [Tier 1](2026-05-05-tier-1-gpio-tdd-design.md) · [Tier 2](2026-05-05-tier-2-sensor-tdd-design.md) · [Tier 3](2026-05-05-tier-3-networked-design.md) · [Tier 4](2026-05-05-tier-4-full-chip-design.md) |

## 1. Mission

`esp32-pio-emulator` is a **behavioral simulator for ESP32 firmware that runs natively on a developer's machine**. It compiles the user's unmodified Arduino-framework sketch against host-side fakes of the ESP32 hardware abstraction layer, then exposes two test surfaces:

- **In-process C++ surface (Unity, via PlatformIO `native` env):** fast unit tests against the fake runtime — assert on GPIO levels, serial output, peripheral interactions, virtual time.
- **Out-of-process Python surface (`pytest-embedded` service plugin):** scenario tests that drive the simulated device from outside, sharing the standard `dut.expect()` API used against Wokwi, QEMU, and real hardware.

The simulator does **not** emulate the Tensilica/RISC-V ISA. It runs the user's code as native host code with the hardware-facing layer substituted. Test feedback is in milliseconds and the implementation stays tractable.

## 2. Value proposition

ESP32 developers today have these test paths, each with sharp tradeoffs:

| Path | Speed | Coverage | OSS / local |
|---|---|---|---|
| Real hardware | Slow (flash + manual setup) | Full | Yes, but requires the board |
| Wokwi | Fast | Broad | No — closed core, hosted |
| QEMU (Espressif fork) | Medium | Partial peripherals; awkward harness | Yes |
| PlatformIO `native` + hand-written mocks | Fast | Only what you mocked | Yes |

`esp32-pio-emulator` targets the **fast + broad + OSS + local** quadrant the existing ecosystem doesn't cover. The success criterion: a developer can do **TDD on ESP32 code** — write a failing test, write the sketch, see it pass, in under one second per cycle, without ever flashing a board.

## 3. Non-goals

- **Cycle-accurate ISA emulation.** That's QEMU's job. We compile native.
- **Replacing real-hardware QA.** Final validation happens on real boards.
- **Replacing Wokwi.** Wokwi is excellent for visual prototyping; we are a test target, not a circuit playground.
- **Multi-MCU support beyond ESP32 in v1.** Architecture leaves room for ESP32 variants (S2/S3/C3/C6) and a future ESP-IDF platform adapter, but v1 implementation targets the Arduino framework on classic ESP32.
- **GUI.** Headless-only, CI-first.

## 4. Architecture

```
┌────────────────────────────────────────────────────────────────────────┐
│                User's ESP32 sketch (unchanged source)                  │
│        digitalWrite(2, HIGH); WiFi.begin(...); Wire.write(...);        │
└──────────────────────────────────┬─────────────────────────────────────┘
                                   │ compiled against
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│  platforms/arduino-esp32/  — fake Arduino.h, WiFi.h, Wire.h, ...       │
│  Each fake forwards every call to the core engine.                     │
└──────────────────────────────────┬─────────────────────────────────────┘
                                   │
                                   ▼
┌────────────────────────────────────────────────────────────────────────┐
│  core/  — VirtualClock, EventLog, PinRegistry, I2CBus, SPIBus,         │
│           UartChannel, PeripheralRegistry, NetworkShim, RtosShim       │
└────────┬───────────────────────────────────────────────────┬───────────┘
         │                                                   │
         │  in-process (linked into test binary)             │  out-of-process (subprocess + IPC)
         ▼                                                   ▼
┌────────────────────────────┐         ┌──────────────────────────────────┐
│ harness/unity/             │         │ harness/pytest_pio_emulator/     │
│ ESP32Sim::expectGpio(2)    │         │ pytest-embedded service plugin   │
│ ESP32Sim::advanceTime(ms)  │         │ — exposes sim as a `Dut` so user │
│ ESP32Sim::uart(0).contains │         │ tests use dut.expect() / etc.    │
│   ("hello")                │         │ Same API as Wokwi/QEMU/real HW.  │
│ ESP32Sim::attachI2C(0x76,  │         │ Adds ESP32-sim-specific fixtures │
│   FakeBMP280{...})         │         │ for time control + peripherals.  │
└────────────────────────────┘         └──────────────────────────────────┘
   "C++ unit-test surface"                "Python scenario-test surface"
   (T1+, fast, in-process)                (T2+, ecosystem leverage)
```

Three layers of code, two test surfaces. The layers communicate through narrow interfaces; the surfaces share the same core state.

### 4.1 Layer responsibilities

- **Sketch** — completely unchanged from real ESP32 code. The user does not write `#ifdef` branches.
- **Platform adapter (`platforms/arduino-esp32/`)** — header-compatible drop-in replacements for the Arduino-ESP32 framework's headers. Every public function and class is implemented as a thin forwarder into the core engine. This is the seam that lets a future `platforms/esp-idf/` plug in.
- **Core engine (`core/`)** — language-and-framework-neutral simulation primitives. No knowledge of `Wire` or `WiFi`; only `I2CBus`, `Network`. The platform adapter translates framework concepts into core concepts.
- **Harnesses (`harness/unity/`, `harness/pytest_pio_emulator/`)** — read core state and let test authors assert on it. The Unity harness is a C++ library linked into the test binary. The pytest-embedded harness is a Python plugin that spawns the test binary as a subprocess and talks to it over a control channel.

### 4.2 Why two surfaces and not one

They serve genuinely different test layers, established as best practice across the ESP32 ecosystem (`pytest-embedded` exists for this reason):

| Layer | Surface | Example test |
|---|---|---|
| Pure logic unit test | Unity in-process | "the CSV parser handles empty fields correctly" |
| Peripheral interaction unit test | Unity in-process | "after `setup()`, GPIO 2 is HIGH within 500 ms" |
| Scenario / system test | pytest-embedded out-of-process | "device boots → connects to WiFi → fetches HTTP → publishes MQTT → recovers from disconnect" |

The two surfaces are not redundant. The Unity surface is fastest and supports rich C++ assertions about internal state. The pytest-embedded surface gives you Python's ecosystem (`pytest-httpserver`, async, fixtures, parametrization) and writes the same test code that runs on real hardware.

## 5. Core abstractions

These are the contracts the entire system depends on. Each is implemented in `core/` and exported through `core/include/esp32sim/`.

### 5.1 `VirtualClock`

Singleton. Owns simulated time. Tests advance the clock explicitly; `delay(ms)` does not sleep — it advances the clock.

```cpp
class VirtualClock {
public:
  uint64_t now_us() const;
  void advance(uint64_t us);                          // runs all due callbacks
  ScheduleHandle schedule_at(uint64_t us, Callback);  // alarms, ISRs
  void cancel(ScheduleHandle);
};
```

### 5.2 `EventLog`

Append-only log of every observable action. Powers assertions and the pytest-embedded surface.

```cpp
struct Event {
  uint64_t timestamp_us;
  EventKind kind;       // GPIO_WRITE, UART_TX, I2C_TXN, WIFI_STATE, ...
  std::variant<...> payload;
};

class EventLog {
public:
  void emit(Event);
  std::vector<Event> since(uint64_t timestamp_us) const;
  // ...iterator, filter-by-kind helpers...
};
```

### 5.3 `PinRegistry`

Maps GPIO pin number → current digital level + mode + attached peripheral (e.g., a fake button driving the pin from outside).

### 5.4 `I2CBus`, `SPIBus`

Bus simulators. Devices register at construction time; transactions go through bus arbitration.

```cpp
class I2CBus {
public:
  void attach(uint8_t addr, std::shared_ptr<I2CDevice>);
  Result write(uint8_t addr, const uint8_t* data, size_t len);
  Result read(uint8_t addr, uint8_t* buf, size_t len);
};

class I2CDevice {
public:
  virtual Result on_write(const uint8_t* data, size_t len) = 0;
  virtual Result on_read(uint8_t* buf, size_t len) = 0;
};
```

### 5.5 `UartChannel`

Per-UART pair of buffers. Sketch writes go to TX; tests can read from TX. Tests can inject into RX; sketch reads consume from RX.

### 5.6 `PeripheralRegistry`

Lookup table for fake devices created by tests. `ESP32Sim::attachI2C(0x76, FakeBMP280{...})` registers a device; the I2C bus consults this registry during `Wire.requestFrom(0x76, ...)`.

### 5.7 `NetworkShim` (T3+)

Abstracts the WiFi/network surface. Two implementations evaluated at T3 entry:
- **Python-mock-server-backed** — sketch network calls forward to Python harness, which routes to mock servers (`pytest-httpserver`, embedded MQTT broker).
- **Real-LWIP-on-host** — sketch links against real LWIP compiled for host; test fixtures provide a tap device or in-process socket pairs.

The choice is captured as a Tier 3 ADR.

### 5.8 `RtosShim` (T4+)

Cooperative pseudo-scheduler that simulates FreeRTOS task scheduling without real preemption. `xTaskCreate`, `xQueueSend`, `xSemaphoreTake` work but tasks yield deterministically at API boundaries. Decision deferred — alternative is FreeRTOS POSIX port.

### 5.9 `PlatformAdapter` interface

The contract every platform implementation must fulfill. `platforms/arduino-esp32/` is the v1 implementation. A future `platforms/esp-idf/` would implement the same contract.

```cpp
struct PlatformAdapter {
  // Lifecycle
  virtual void on_test_setup() = 0;     // resets engine state
  virtual void run_setup() = 0;         // calls user's setup()
  virtual void run_loop_iteration() = 0;// calls user's loop() once
  virtual void on_test_teardown() = 0;
};
```

The user's `setup()`/`loop()` (or `app_main()` for ESP-IDF) are invoked through this contract; tests never call them directly.

## 6. Repository layout

```
esp32-pio-emulator/
├── AGENTS.md                               # operator preferences (T0)
├── CLAUDE.md                               # task-handoff context (T0)
├── README.md                               # entry point (T0)
├── LICENSE                                 # MIT (T0)
├── platformio.ini                          # native + esp32 envs
├── library.json                            # PIO registry metadata
├── docs/                                   # Diátaxis split
│   ├── README.md                           # the map
│   ├── user/                               # ESP32 devs writing tests
│   │   ├── tutorials/                      # "your first GPIO test in 5 min"
│   │   ├── how-to/                         # "how to fake an I2C sensor"
│   │   ├── reference/                      # API reference
│   │   └── explanation/                    # "why virtual time"
│   ├── dev/                                # framework contributors
│   │   ├── how-to/
│   │   ├── reference/
│   │   └── explanation/
│   ├── decisions/                          # ADRs
│   └── superpowers/specs/                  # design specs (this folder)
├── core/
│   ├── include/esp32sim/                   # public headers
│   └── src/                                # implementations
├── platforms/
│   └── arduino-esp32/
│       ├── include/                        # fake Arduino.h, WiFi.h, ...
│       └── src/
├── harness/
│   ├── unity/
│   │   ├── include/esp32sim_unity/
│   │   └── src/
│   └── pytest_pio_emulator/                # T2+
│       ├── pytest_pio_emulator/__init__.py
│       └── pyproject.toml
├── peripherals/                            # T2+ stateful fakes
│   ├── bmp280/
│   ├── mcp23017/
│   └── ...
├── examples/                               # end-to-end at every tier
│   ├── 01-blink/                           # T1
│   ├── 02-button-debounce/                 # T1
│   ├── 03-bmp280-logger/                   # T2
│   ├── 04-mqtt-temperature/                # T3
│   └── 05-deep-sleep-ble-provision/        # T4
├── test/                                   # framework's own self-tests
│   ├── core/
│   ├── platforms/
│   └── peripherals/
└── .github/workflows/                      # CI
    └── ci.yml
```

The split between `core/`, `platforms/`, `harness/`, and `peripherals/` is the architectural seam. Anything that knows about `Wire` lives in `platforms/`; anything that knows about I2C-as-a-protocol lives in `core/`; anything that knows about a specific I2C chip lives in `peripherals/`; anything that helps you test lives in `harness/`.

## 7. Tier roadmap

| Tier | Codename | Surfaces | Deliverable | Acceptance |
|---|---|---|---|---|
| **T0** | Skeleton | — | Repo bootstrap, AGENTS.md, CLAUDE.md, Diátaxis docs scaffold, PlatformIO project, GH Actions CI | `pio test -e native` runs a trivial pass test |
| **T1** | GPIO TDD | Unity | `pinMode`, `digital{Read,Write}`, `Serial`, `millis`, `micros`, `delay`, virtual clock, `ESP32Sim::*` assertion API | Test-drive an LED blinker, a serial echo, a button debouncer — all without hardware |
| **T2** | Sensor TDD | Unity + pytest-embedded plugin α | I2C, SPI, ADC, PWM (`ledc`), interrupts, hardware timers, first stateful peripheral fakes (BMP280, MCP23017) | BMP280 logger sketch tested via Unity unit tests AND a pytest-embedded scenario test |
| **T3** | Networked | pytest-embedded leads | WiFi (connect/RSSI/disconnect), `WiFiClient`/`WiFiServer`, HTTP, MQTT, NTP. Decision point on networking impl recorded as ADR. | Sketch connects to "WiFi", fetches HTTP from a test fixture, publishes to MQTT — fully offline in CI |
| **T4** | Full chip | Both | LittleFS, SPIFFS, NVS/Preferences, deep-sleep, RMT, dual-core/FreeRTOS scheduling, BT Classic, BLE | Reference project: temperature logger w/ deep-sleep + BLE provisioning + WiFi MQTT, fully tested offline |

Each tier is its own brainstorm → spec → plan → execute cycle. Per-tier specs live alongside this one.

## 8. Quality bar

Per AGENTS.md (`Per-step verify-and-document`, `Documentation currency`):

- Every implementation step ends with: (1) verification command run + green, (2) every doc the change affects updated in the same commit series, (3) commit + push.
- No tier ships until its reference example passes in CI.
- Spec drift policy: if reality contradicts spec, commit a spec update *first*, then adapt code.
- Small, reviewable commits. One concept per commit.
- TDD by default for original code; the framework's fakes have unit tests.

## 9. Documentation strategy

[Diátaxis](https://diataxis.fr/) — four docs categories × two audiences:

| Audience | Tutorials | How-to | Reference | Explanation |
|---|---|---|---|---|
| **User** (test author) | "Your first GPIO test in 5 min" (T1) | "Fake an I2C sensor" (T2), "Test a WiFi sketch" (T3) | API reference for `ESP32Sim::*` and pytest fixtures | "Why virtual time", "Fake vs mock vs emulate" |
| **Dev** (framework contributor) | "Set up your dev environment" (T0) | "Add a new peripheral fake" (T2), "Add a platform adapter" (T2) | Internal architecture, ABI contracts | Design rationale, link to ADRs |

Documentation is created or updated in the **same commit** as the code change. `docs/README.md` is the navigation map and is updated whenever a new doc lands.

## 10. Meta-testing strategy

The framework itself is tested:
- **`test/core/`** — unit tests for `VirtualClock`, `EventLog`, `I2CBus`, `SPIBus`, `UartChannel` against direct C++ usage (no sketch).
- **`test/platforms/arduino-esp32/`** — adapter conformance tests that exercise the fake `Arduino.h` from a tiny sketch and verify the resulting events.
- **`test/peripherals/`** (T2+) — each fake peripheral has datasheet-conformance tests (e.g., "BMP280 fake responds to register 0xD0 with chip ID 0x58").
- **`examples/`** — end-to-end smoke tests run in CI on every PR. An example failing means a tier regression.

CI runs the full matrix on Linux and macOS (ESP32 dev population is overwhelmingly on these).

## 11. Open decisions (deferred)

| # | Decision | Defer until | Capture as |
|---|---|---|---|
| D1 | T3 networking impl: Python mock servers vs real-LWIP-on-host | Tier 3 entry | ADR |
| D2 | T4 RTOS impl: cooperative pseudo-scheduler vs FreeRTOS POSIX port | Tier 4 entry | ADR |
| D3 | Project license confirmation (MIT assumed) | Pre-public-release | LICENSE file + AGENTS.md note |
| D4 | PlatformIO library registry namespace (`esp32-pio-emulator`) | Pre-public-release | `library.json` |
| D5 | Whether to support ESP32 variants (S2/S3/C3/C6) explicitly in v1 | T2 entry (peripherals start to differ) | ADR |
| D6 | Code formatter / linter choice (clang-format style, Python ruff vs black) | T0 implementation | settled in T0 plan |
| D7 | T4 BLE simulation depth: stub-level vs behavioral (with Python BLE peer fixture) | Tier 4 entry | ADR |

## 12. Glossary

- **Behavioral simulator** — software that mimics observable hardware behavior without emulating CPU instructions. Contrast with **emulator** (e.g., QEMU) which executes the target ISA.
- **HAL (Hardware Abstraction Layer)** — the layer of API calls (`digitalWrite`, `Wire.beginTransmission`, etc.) that we intercept.
- **Sketch** — Arduino-framework user code with `setup()` and `loop()`, possibly across multiple `.cpp` files.
- **Dut (Device Under Test)** — pytest-embedded's standard interface for talking to the device (real or simulated).
- **Tier** — a phased capability set (T0 → T4); each tier ships standalone before the next begins.
- **Surface** — a test API exposed to users. We have two: in-process Unity and out-of-process pytest-embedded.
- **Fake** vs **mock** — a *fake* implements working behavior (a real-ish BMP280 that returns sensible data); a *mock* records calls for verification (`ArduinoFake`-style).

## 13. Changelog

| Date | Version | Change |
|---|---|---|
| 2026-05-05 | v0.1 | Initial draft after brainstorming + OSS research |
