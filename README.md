# esp32-pio-emulator

> **🚀 v1.0 shipped** (all four tiers complete) · License: MIT · ESP32-S3 primary target · gnu++17

A **behavioral simulator for ESP32 firmware** that runs natively on your laptop. Compile your unmodified Arduino sketch against host-side fakes of the ESP32 hardware abstraction layer, run Unity tests, assert on what your code makes the chip do — without flashing a board, in milliseconds per test cycle.

```cpp
// Your sketch — unchanged from real hardware
void setup() { pinMode(2, OUTPUT); }
void loop()  { digitalWrite(2, HIGH); delay(500); digitalWrite(2, LOW); delay(500); }

// Your test
void test_blinks(void) {
    esp32sim::Sim::reset();
    esp32sim::Sim::runSetup();
    esp32sim::Sim::advanceMs(500);
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(2).level());
}
```

`pio test -e native` → green in 1.5 s. No hardware. No flashing.

---

## Why this exists

The ESP32 testing landscape today has **fast-but-shallow** (hand-rolled mocks), **slow-but-broad** (real hardware / Wokwi / QEMU), or **fast-but-closed** (Wokwi's hosted simulator). This project covers the **fast + broad + open-source + locally-runnable** quadrant. Catch logic bugs, protocol bugs, sequencing bugs, and timing bugs at the millisecond scale before you flash. Hardware QA still has a job — but it's the *final* sign-off, not the inner TDD loop.

---

## 5-minute quickstart

### 1. Make a project

```bash
mkdir my-test-project && cd my-test-project
mkdir -p src test/test_blink
```

### 2. Write your sketch — the way you'd write it for real hardware

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

First run is slow (PlatformIO clones the framework from GitHub). Subsequent runs are sub-second.

```
test/test_blink/test_blink.cpp:N: test_led_starts_low    [PASSED]
test/test_blink/test_blink.cpp:N: test_led_toggles       [PASSED]
2 test cases: 2 succeeded
```

That's it. You're testing real ESP32 code on your laptop in milliseconds.

---

## Don't have a recent PlatformIO?

System `pio` from `apt`/`brew` is often too old (4.x); we need 6.x. Use a project-local venv:

```bash
python3 -m venv .venv
.venv/bin/pip install platformio
.venv/bin/pio test -e native
```

---

## What's supported (v1.0)

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

---

## Test-side API cheat sheet

What you call from inside your tests to drive the sim and observe state:

```cpp
#include <esp32sim_unity/esp32sim.h>

esp32sim::Sim::reset();              // call from setUp() — clears all sim state
esp32sim::Sim::runSetup();           // calls your sketch's setup()
esp32sim::Sim::runLoop(int n = 1);   // calls your sketch's loop() n times
esp32sim::Sim::runUntil(predicate, timeoutMs);  // loop+advance until predicate true
esp32sim::Sim::advanceMs(ms);        // advance virtual clock — does not sleep
esp32sim::Sim::advanceUs(us);
esp32sim::Sim::nowMs();              // virtual time

esp32sim::Sim::gpio(pin).level();             // read pin level
esp32sim::Sim::gpio(pin).setLevel(v);         // simulate external driver
esp32sim::Sim::gpio(pin).pulse(level, ms);    // pulse + auto-restore (for buttons)

esp32sim::Sim::uart(0).drainTx();    // returns what Serial.print()ed since last drain
esp32sim::Sim::uart(0).inject("data"); // give Serial.read() bytes to consume

esp32sim::Sim::events().kind(GPIO_WRITE).pin(2).count();  // assertion query

// Attach a fake peripheral
auto rtc = std::make_shared<esp32sim::peripherals::FakeDS3231>();
rtc->setTime(12, 34, 56);
esp32sim::I2CBus::for_index(0).attach(0x68, rtc);

// Pre-seed an HTTP response for the sketch to fetch
esp32sim::HttpResponse r{200, "{\"key\":1}", {}};
esp32sim::Network::instance().seed_http_response("https://api.example.com/x", r);

// Drive an incoming MQTT message
esp32sim::Network::instance().mqtt_deliver("control/led", "ON");
```

Full reference: [`docs/user/reference/sim-api.md`](docs/user/reference/sim-api.md).

---

## More examples

Seven complete worked examples in `examples/`, each runnable with `pio test -e native`:

| Example | What it tests | APIs |
|---|---|---|
| [`01-blink`](examples/01-blink/) | Classic 1Hz LED blink | GPIO + virtual time |
| [`02-button-debounce`](examples/02-button-debounce/) | 50ms debouncer | GPIO + INPUT_PULLUP + millis |
| [`03-serial-echo`](examples/03-serial-echo/) | Uppercase echo | Serial read/write |
| [`04-rtc-moisture-logger`](examples/04-rtc-moisture-logger/) | RTC + ADC logging | I2C + DS3231 + analogRead + Serial |
| [`05-pwm-fade`](examples/05-pwm-fade/) | LEDC sweep | PWM/LEDC |
| [`06-mqtt-temperature`](examples/06-mqtt-temperature/) | WiFi → HTTP config → MQTT publish | WiFi + HTTPClient + PubSubClient |
| [`07-deep-sleep-mqtt`](examples/07-deep-sleep-mqtt/) | Kitchen-sink: NVS-stored creds, BLE provisioning fallback, MQTT publish, 60s deep sleep | All of T4 |

---

## What this catches (and what it doesn't)

**Catches:** logic bugs, off-by-one errors, wrong pin / wrong register / wrong topic / wrong URL, state-machine bugs, debounce timing bugs, NVS-persistence bugs, sleep-cycle behavior, ISR mode mismatches, MQTT topic / payload format errors, application-layer protocol bugs.

**Doesn't catch:** real RF / electrical effects, dual-core preemption races, real flash wear-leveling, real ISR timing edge cases, Xtensa-architecture-specific compiler bugs, real broker / TLS handshake quirks. Those remain real-hardware QA's job.

Full list: [`docs/user/explanation/what-this-does-and-doesnt-catch.md`](docs/user/explanation/what-this-does-and-doesnt-catch.md).

---

## Status

| Tier | Capability | Status |
|---|---|---|
| T0 | Skeleton | ✓ v0.1.0 |
| T1 | GPIO TDD | ✓ v0.2.0 |
| T2 | Sensor TDD + pytest-embedded plugin | ✓ v0.3.0 |
| T3 | Networked ESP32 (WiFi, HTTP, MQTT) | ✓ v0.4.0 |
| T4 | Full chip (filesystem, NVS, deep-sleep, BLE, RTOS) | ✓ **v1.0.0** |

Post-v1.0 work tracked in [`CLAUDE.md`](CLAUDE.md).

---

## Documentation

- **Tutorials** — [`docs/user/tutorials/`](docs/user/tutorials/) — start here.
- **How-to guides** — [`docs/user/how-to/`](docs/user/how-to/) — concrete recipes (debounce, I2C sensor, WiFi sketch, MQTT, deep-sleep, BLE provisioning, filesystem, etc.).
- **API reference** — [`docs/user/reference/`](docs/user/reference/).
- **Explanations** — [`docs/user/explanation/`](docs/user/explanation/) — *why* virtual time, what we catch, framework vs ArduinoFake.
- **Specs + ADRs** — [`docs/superpowers/specs/`](docs/superpowers/specs/), [`docs/decisions/`](docs/decisions/).
- **Contributing** — [`docs/dev/tutorials/setting-up-dev-environment.md`](docs/dev/tutorials/setting-up-dev-environment.md).

---

## How is this different from…

- **Wokwi** — circuit-focused, hosted, closed-source core. Great for visual prototyping; we're built for fast headless TDD.
- **QEMU (Espressif fork)** — emulates the Tensilica/RISC-V ISA. Slower and more faithful; we trade fidelity for speed (milliseconds per test cycle vs seconds).
- **EpoxyDuino** — compiles Arduino code on Linux but stubs are deliberately empty. We provide *behavioral* fakes that respond.
- **ArduinoFake** — call-recording mocks; asserts on which calls happened. We provide behavioral simulation; we assert on observable state. Different abstraction; details in [`our-framework-vs-arduinofake.md`](docs/user/explanation/our-framework-vs-arduinofake.md).

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

See [`docs/user/how-to/use-pytest-embedded.md`](docs/user/how-to/use-pytest-embedded.md). Alpha-scope is stdout-only; richer control deferred to T2.5 per [ADR-0004](docs/decisions/0004-pytest-plugin-control-channel.md).

---

## License

MIT — see [LICENSE](LICENSE).
