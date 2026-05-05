# esp32-pio-emulator

> **🚀 v1.2** · License: MIT · ESP32-S3 primary target · gnu++17 · 187 tests passing

A **behavioral simulator for ESP32 firmware** that runs natively on your laptop. Compile your unmodified Arduino sketch against host-side fakes of the ESP32 hardware abstraction layer. Two ways to verify your firmware:

1. **Strict mode (zero authoring)** — drop your sketch in, the sim watches it run, reports chip-contract violations against ~21 well-known ESP32 / Arduino-ESP32 rules. Severity-classified (ERROR vs WARNING). [Jump to example.](#zero-authoring-verification-strict-mode)
2. **Authored TDD** — write Unity tests against your sketch's `setup()` / `loop()` and assert on observable behavior — pin levels, serial output, peripheral interactions, virtual time. Sub-second feedback. [Jump to example.](#5-minute-quickstart-authored-tests)

Both surfaces share the same simulator, the same fakes, and the same test runtime. Pick whichever fits the question you're asking.

---

## Why this exists

The ESP32 testing landscape has **fast-but-shallow** (hand-rolled mocks), **slow-but-broad** (real hardware / Wokwi / QEMU), or **fast-but-closed** (Wokwi's hosted simulator). This project covers the **fast + broad + open-source + locally-runnable** quadrant. Catch logic bugs, protocol bugs, sequencing bugs, and timing bugs at the millisecond scale before you flash. Hardware QA still has a job — but it's the *final* sign-off, not the inner TDD loop.

---

## Zero-authoring verification (strict mode)

The sim already knows what ESP32 firmware can and cannot do. Enable strict mode and the chip's contract becomes the oracle — you don't write any assertions:

```cpp
#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) {
    esp32sim::Sim::reset();
    esp32sim::Strict::instance().enable();   // ← turn on the contract checker
}
void tearDown(void) {}

void test_sketch_obeys_chip_contract(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop(20);
    auto& s = esp32sim::Strict::instance();
    s.print_report();                                         // log everything
    if (s.has_errors()) TEST_FAIL_MESSAGE("contract errors"); // fail only on ERRORs
    // warnings stay in the log as info; they don't fail the test
}
```

If your sketch misbehaves, the report is grep-able and severity-tagged:

```
[strict-mode] 4 violation(s): 2 error(s), 2 warning(s)
  ERRORS (must-fix):
    [ESP_SIM_E001 @ t=0 us] digitalWrite on GPIO 4 with mode != OUTPUT —
      must call pinMode(4, OUTPUT) before digitalWrite
    [ESP_SIM_E021 @ t=12000 us] Wire.beginTransmission(0x99) —
      I2C addresses must fit in 7 bits (0x00..0x7F)
  WARNINGS (recommendations):
    [ESP_SIM_E007 @ t=500 us] pinMode on GPIO 19 which is wired to the on-chip
      USB-JTAG bridge on ESP32-S3 — using it as GPIO disables USB-JTAG
      debugging on this board
    [ESP_SIM_E006 @ t=200 us] pinMode on GPIO 0 which is a strapping pin on
      ESP32-S3 — may interfere with boot/flash mode selection
```

Every code is documented in [`docs/user/reference/strict-mode.md`](docs/user/reference/strict-mode.md) with the authoritative source citation (datasheet section, ESP-IDF guide, etc.).

### Rules at a glance

**ERROR** (12 — definitely broken; will misbehave on real hardware):

| Code | Rule |
|---|---|
| E001 | `digitalWrite` without prior `pinMode(OUTPUT)` |
| E002 | GPIO operation on flash-reserved pin (26-32 on ESP32-S3 — bricks device) |
| E003 | Pin number out of range (S3 max = GPIO 48) |
| E020 | `Wire.write` outside `beginTransmission`/`endTransmission` |
| E021 | I2C address > 0x7F (7-bit space violation) |
| E022 | Nested `Wire.beginTransmission` |
| E040 | `delayMicroseconds(>16383)` (real hw treats as ~zero) |
| E051 | `HTTPClient::GET`/`POST` without prior `begin(url)` |
| E060 | `Preferences::putString`/etc. without prior `begin(ns)` |
| E070 | `ledcWrite` without prior `ledcSetup` |
| E071 | LEDC channel > 7 (S3 has channels 0-7 only) |
| E080/E081 | BLE `createServer`/`createService` before `BLEDevice::init` |

**WARNING** (9 — fragile or suboptimal but works):

| Code | Rule |
|---|---|
| E004 | `analogRead` on non-ADC pin (S3 ADC1: GPIO 1-10, ADC2: GPIO 11-20) |
| E006 | `pinMode` on a strapping pin (GPIO 0/3/45/46) — boot-time risk |
| E007 | `pinMode` on GPIO 19/20 — USB-JTAG D-/D+ on ESP32-S3 |
| E010 | Serial use before `Serial.begin` (arduino-esp32 buffers + auto-flushes) |
| E050 | `WiFi.localIP`/`RSSI`/`SSID` before `WiFi.begin` |
| E052 | `PubSubClient::publish` on disconnected client (silent drop) |
| E061 | NVS namespace > 15 chars (truncated) |

> Real-world finding: rule **E007** was added in v1.2 after verifying [`fresh-fx59/iot-yc-water-the-flowers`](https://github.com/fresh-fx59/iot-yc-water-the-flowers), which uses GPIO 19 as a water-level sensor pin — works fine, but disables USB-JTAG debugging on the board. The harness now flags this so future contributors won't re-discover it the painful way.

### What strict mode catches and what it doesn't

**Catches:** chip-contract violations (forbidden API ordering, illegal pin choices, protocol violations, hardware limits, lifecycle errors).

**Doesn't catch:** **intent-level bugs.** "Publishes to the wrong topic", "watering algorithm runs too long", "off-by-one in state machine" — these need authored assertions because the framework can't infer your firmware's intent. Strict mode is the *floor*, authored TDD is the *ceiling*.

See [`docs/user/explanation/what-this-does-and-doesnt-catch.md`](docs/user/explanation/what-this-does-and-doesnt-catch.md) for the honest list.

---

## 5-minute quickstart (authored tests)

If you have intent-level behavior to verify (correct values, sequencing, computed outputs), write Unity tests directly.

### 1. Make a project

```bash
mkdir my-test-project && cd my-test-project
mkdir -p src test/test_blink
```

### 2. Write your sketch — exactly as you would for real hardware

`src/main.cpp`:

```cpp
#include <Arduino.h>

void setup() {
    pinMode(2, OUTPUT);
}

void loop() {
    digitalWrite(2, HIGH);
    delay(500);
    digitalWrite(2, LOW);
    delay(500);
}
```

### 3. Configure PlatformIO

`platformio.ini`:

```ini
[env:native]
platform = native
test_framework = unity

; Three knobs are mandatory when you depend on us via lib_deps:
lib_compat_mode = off       ; native env has no framework; turn off PIO's compat filter
test_build_src = true       ; compile your sketch into the test binary
lib_deps = https://github.com/fresh-fx59/esp32-pio-emulator
build_flags = -std=gnu++17
```

### 4. Write your test

`test/test_blink/test_blink.cpp`:

```cpp
#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void)    { esp32sim::Sim::reset(); }
void tearDown(void) {}

void test_led_starts_low(void) {
    esp32sim::Sim::runSetup();
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());
}

void test_led_toggles(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(2).level());
    esp32sim::Sim::advanceMs(500);
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_led_starts_low);
    RUN_TEST(test_led_toggles);
    return UNITY_END();
}
```

### 5. Run it

```bash
pio test -e native
```

```
test/test_blink/test_blink.cpp:N: test_led_starts_low    [PASSED]
test/test_blink/test_blink.cpp:N: test_led_toggles       [PASSED]
2 test cases: 2 succeeded
```

That's it — real ESP32 code under test on your laptop, sub-second per cycle.

---

## Don't have a recent PlatformIO?

System `pio` from `apt`/`brew` is often too old (4.x); we need 6.x. Use a project-local venv:

```bash
python3 -m venv .venv
.venv/bin/pip install platformio
.venv/bin/pio test -e native
```

The system `pio` 4.3.4 on Ubuntu 24+ / Python 3.12 crashes on `cli.resultcallback`. The venv path avoids it.

---

## Combine both approaches

Strict mode and authored tests in the same test binary work fine — they share the simulator state. Typical pattern: *one* zero-authoring test that runs the sketch and fails on contract errors, *plus* targeted authored tests for intent-level behavior.

```cpp
// 1. Strict-mode contract check (zero-authoring)
void test_sketch_obeys_chip_contract(void) {
    esp32sim::Sim::reset();
    esp32sim::Strict::instance().enable();
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop(20);
    auto& s = esp32sim::Strict::instance();
    s.print_report();
    if (s.has_errors()) TEST_FAIL_MESSAGE("strict-mode errors");
}

// 2. Authored: assert on the actual MQTT topic the sketch publishes to
void test_publishes_to_correct_topic(void) {
    esp32sim::Sim::reset();
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();
    auto& publishes = esp32sim::Network::instance().mqtt_publishes();
    TEST_ASSERT_EQUAL_STRING("sensors/temp", publishes[0].topic.c_str());
}
```

---

## What's supported

A user-side cheat sheet of every Arduino API that *behaves* in the sim — not just compiles, *behaves*:

| Surface | APIs |
|---|---|
| **GPIO + timing** | `pinMode`, `digital{Read,Write}`, `millis`, `micros`, `delay`, `delayMicroseconds`, `yield`, `attachInterrupt` (RISING/FALLING/CHANGE/ONHIGH/ONLOW), `digitalPinToInterrupt`, `detachInterrupt` |
| **UART / Serial** | `Serial`, `Serial1`, `Serial2`: `begin`, `print`, `println`, `printf`, `write`, `read`, `available`, `peek`, `flush` |
| **I2C** | `Wire`, `Wire1`: full `TwoWire` API. Pluggable `I2CDevice` peripherals. |
| **SPI** | `SPI`: `beginTransaction`, `transfer`, `transfer16`, buffer transfer, CS routing |
| **ADC** | `analogRead`, `analogReadResolution`, `analogSetAttenuation` |
| **PWM/LEDC** | `ledcSetup`, `ledcAttachPin`, `ledcWrite`, `ledcRead`, `analogWrite` (16 channels) |
| **HW timers** | `timerBegin`, `timerAlarmEnable`, `timerAttachInterrupt`, `timerAlarmWrite` |
| **Peripheral fakes** | `FakeDS3231` (RTC, 0x68), `FakeBMP280` (pressure+temp, 0x76), `FakeMCP23017` (16-bit I/O expander, 0x20) |
| **WiFi** | `WiFi.begin`, `disconnect`, `status`, `RSSI`, `localIP`, `SSID`, `WL_*` enum, `IPAddress` |
| **HTTP** | `HTTPClient`: `begin`, `GET`, `POST`, `getString`, `addHeader`. Pre-seed responses with `Sim::Network::seed_http_response(url, ...)` |
| **MQTT** | `PubSubClient`: `setServer`, `connect`, `publish`, `subscribe`, `loop`. Drive incoming messages with `Sim::Network::mqtt_deliver(topic, payload)` |
| **NVS** | `Preferences`: `begin`/`putString`/`getString`/`putUInt`/`getUInt`/`putInt`/`getInt`/`putBool`/`getBool`/`clear`/`remove`/`isKey` |
| **Filesystem** | LittleFS/SPIFFS-compatible behavior via `esp32sim::FileSystem` |
| **Deep sleep** | `esp_sleep_enable_timer_wakeup`, `esp_deep_sleep_start`, `esp_sleep_get_wakeup_cause` |
| **FreeRTOS** | `xTaskCreate`, `xTaskCreatePinnedToCore`, `vTaskDelay`, `xQueueCreate/Send/Receive`, `xSemaphoreCreate{Binary,Counting,Mutex}`, `Take`, `Give` (cooperative model — no real preemption) |
| **BLE** | `BLEDevice::init`, `createServer`, `createService`, `BLECharacteristic` (set/get/notify), `BLEAdvertising` (stub-level — records intent; no GATT peer simulation) |
| **Strict mode** (v1.1+) | 21 chip-contract rules across all of the above; ERROR vs WARNING severity |

---

## Test-side API cheat sheet

What you call from inside your tests to drive the sim and observe state:

```cpp
#include <esp32sim_unity/esp32sim.h>

// Lifecycle / time
esp32sim::Sim::reset();              // call from setUp() — clears all sim state
esp32sim::Sim::runSetup();           // calls your sketch's setup()
esp32sim::Sim::runLoop(int n = 1);   // calls your sketch's loop() n times
esp32sim::Sim::runUntil(predicate, timeoutMs);  // loop+advance until predicate true
esp32sim::Sim::advanceMs(ms);        // advance virtual clock — does not sleep
esp32sim::Sim::nowMs();              // virtual time

// GPIO
esp32sim::Sim::gpio(pin).level();             // read pin level
esp32sim::Sim::gpio(pin).setLevel(v);         // simulate external driver
esp32sim::Sim::gpio(pin).pulse(level, ms);    // pulse + auto-restore (for buttons)

// Serial
esp32sim::Sim::uart(0).drainTx();    // returns what Serial.print()ed since last drain
esp32sim::Sim::uart(0).inject("data"); // give Serial.read() bytes to consume

// Event log assertions
esp32sim::Sim::events().kind(GPIO_WRITE).pin(2).count();

// Attach a fake peripheral
auto rtc = std::make_shared<esp32sim::peripherals::FakeDS3231>();
rtc->setTime(12, 34, 56);
esp32sim::I2CBus::for_index(0).attach(0x68, rtc);

// Pre-seed an HTTP response
esp32sim::HttpResponse r{200, "{\"key\":1}", {}};
esp32sim::Network::instance().seed_http_response("https://api.example.com/x", r);

// Drive an incoming MQTT message
esp32sim::Network::instance().mqtt_deliver("control/led", "ON");

// Strict mode (v1.1+)
esp32sim::Strict::instance().enable();
esp32sim::Strict::instance().has_errors();    // bool
esp32sim::Strict::instance().has_warnings();  // bool
esp32sim::Strict::instance().errors();        // std::vector<Violation>
esp32sim::Strict::instance().warnings();
esp32sim::Strict::instance().print_report();  // grouped by severity
```

Full reference: [`docs/user/reference/sim-api.md`](docs/user/reference/sim-api.md) and [`docs/user/reference/strict-mode.md`](docs/user/reference/strict-mode.md).

---

## Worked examples

Seven complete reference projects in `examples/`, each runnable via `pio test -e native`:

| Example | What it tests | APIs |
|---|---|---|
| [`01-blink`](examples/01-blink/) | Classic 1Hz LED blink (3 tests) | GPIO + virtual time |
| [`02-button-debounce`](examples/02-button-debounce/) | 50ms debouncer (3 tests) | GPIO + INPUT_PULLUP + millis |
| [`03-serial-echo`](examples/03-serial-echo/) | Uppercase echo (3 tests) | Serial read/write |
| [`04-rtc-moisture-logger`](examples/04-rtc-moisture-logger/) | RTC + ADC logging (2 tests) | I2C + DS3231 + analogRead + Serial |
| [`05-pwm-fade`](examples/05-pwm-fade/) | LEDC sweep (3 tests) | PWM/LEDC |
| [`06-mqtt-temperature`](examples/06-mqtt-temperature/) | WiFi → HTTP config → MQTT publish (3 tests) | WiFi + HTTPClient + PubSubClient |
| [`07-deep-sleep-mqtt`](examples/07-deep-sleep-mqtt/) | Kitchen-sink: NVS-stored creds, BLE provisioning fallback, MQTT publish, 60s deep sleep (3 tests) | All of T4 |

---

## Pytest-embedded scenario tests (alpha)

In addition to in-process Unity tests, you can drive your sketch from outside via subprocess + stdout, the same way `pytest-embedded` works for real hardware / Wokwi / QEMU:

```bash
pip install -e harness/pytest_pio_emulator
```

```python
def test_sketch_publishes_temp(dut):
    dut.expect("READY")
    m = dut.expect_re(r"sensor=(\d+)")
    assert int(m.group(1)) > 0
```

See [`docs/user/how-to/use-pytest-embedded.md`](docs/user/how-to/use-pytest-embedded.md). Alpha-scope is stdout-only; richer control deferred per [ADR-0004](docs/decisions/0004-pytest-plugin-control-channel.md).

---

## How is this different from…

- **Wokwi** — circuit-focused, hosted, closed-source core. Great for visual prototyping; we're built for fast headless TDD with autonomous chip-contract checks.
- **QEMU (Espressif fork)** — emulates the Tensilica/RISC-V ISA. Slower and more faithful; we trade fidelity for speed (milliseconds per test cycle vs seconds).
- **EpoxyDuino** — compiles Arduino code on Linux but stubs are deliberately empty. We provide *behavioral* fakes that respond.
- **ArduinoFake** — call-recording mocks; asserts on which calls happened. We provide behavioral simulation; we assert on observable state. Different abstraction; details in [`our-framework-vs-arduinofake.md`](docs/user/explanation/our-framework-vs-arduinofake.md).
- **ESP-IDF runtime sanitizers** — heap poisoning / watchdog / brownout detection that fires on real hardware. We replicate the *concept* (run-time contract enforcement) in the sim so it fires before you flash.
- **clang-tidy / cppcheck** — static analysis at compile time. Complementary; we catch runtime contract violations they can't see.

---

## Status

| Tier / version | Capability | Released |
|---|---|---|
| T0 — Skeleton | repo bootstrap + CI + Diátaxis docs | v0.1.0 |
| T1 — GPIO TDD | GPIO + Serial + timing + interrupts + `ESP32Sim::*` | v0.2.0 |
| T2 — Sensor TDD | I2C, SPI, ADC, PWM, hardware timers + DS3231/BMP280/MCP23017 fakes + pytest plugin alpha | v0.3.0 |
| T3 — Networked | WiFi, HTTPClient, PubSubClient (event-recording) | v0.4.0 |
| T4 — Full chip | NVS/Preferences, LittleFS/SPIFFS, deep-sleep, cooperative FreeRTOS shim, BLE stub | **v1.0.0** |
| Strict mode | autonomous chip-contract enforcement, 21 rules, severity classified | **v1.1.0 → v1.2.0** |

Future work (T2.5/T3.5/T4.5/v1.3+) tracked in [`CLAUDE.md`](CLAUDE.md).

---

## Documentation

- **Tutorials** — [`docs/user/tutorials/`](docs/user/tutorials/) — start here.
- **How-to guides** — [`docs/user/how-to/`](docs/user/how-to/) — concrete recipes (debounce, I2C sensor, WiFi sketch, MQTT, deep-sleep, BLE provisioning, filesystem, strict mode, pytest-embedded).
- **API reference** — [`docs/user/reference/`](docs/user/reference/) including [`strict-mode.md`](docs/user/reference/strict-mode.md) (every error code with source citation).
- **Explanations** — [`docs/user/explanation/`](docs/user/explanation/) — *why* virtual time, what we catch, framework vs ArduinoFake, fake vs mock vs emulate.
- **Specs + ADRs** — [`docs/superpowers/specs/`](docs/superpowers/specs/), [`docs/decisions/`](docs/decisions/).
- **Contributing** — [`docs/dev/tutorials/setting-up-dev-environment.md`](docs/dev/tutorials/setting-up-dev-environment.md).

---

## License

MIT — see [LICENSE](LICENSE).
