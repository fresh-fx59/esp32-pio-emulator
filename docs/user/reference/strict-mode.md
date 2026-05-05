# Strict mode reference (v1.1)

Strict mode enables **autonomous chip-contract enforcement**: when on, the
sim's existing fakes upgrade their silent failures to recorded violations
against well-known ESP32 / Arduino-ESP32 contract rules. **No test authoring
needed** — drop a sketch in, enable strict mode, and the chip's contract is
the oracle.

## Enabling

```cpp
#include <esp32sim_unity/esp32sim.h>

void setUp(void) {
    esp32sim::Sim::reset();
    esp32sim::Strict::instance().enable();
}

void test_my_sketch_is_well_behaved(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop(10);
    TEST_ASSERT_EQUAL_size_t(0, esp32sim::Strict::instance().count());
    if (esp32sim::Strict::instance().any()) {
        esp32sim::Strict::instance().print_report();
        TEST_FAIL_MESSAGE("strict-mode violations detected");
    }
}
```

The default is **off** — existing tests are unaffected unless they call
`Strict::instance().enable()` explicitly.

## Rule index

Every rule has a unique error code (`ESP_SIM_EXXX`) so users can grep for
remediation guidance.

### GPIO

| Code | Rule | Source |
|---|---|---|
| `ESP_SIM_E001` | `digitalWrite` called on a pin without prior `pinMode(pin, OUTPUT)` | [arduino-esp32 GPIO docs](https://docs.espressif.com/projects/arduino-esp32/en/latest/), [forum reports](https://www.esp32.com/viewtopic.php?t=37347) |
| `ESP_SIM_E002` | GPIO operation on a flash-reserved pin (GPIO 26-32 on ESP32-S3 — would corrupt flash) | [ESP32-S3 datasheet §3.4](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf) |
| `ESP_SIM_E003` | Pin number out of range (ESP32-S3 max is GPIO 48) | [ESP-IDF GPIO ref](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html) |
| `ESP_SIM_E004` | `analogRead` on a non-ADC-capable pin (ESP32-S3 ADC1: GPIO 1-10, ADC2: GPIO 11-20) | [ESP32-S3 datasheet ADC section](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf) |
| `ESP_SIM_E006` | `pinMode` on a strapping pin (ESP32-S3: GPIO 0, 3, 45, 46) — may interfere with boot | [ESP32-S3 strapping pins](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html) |

### Serial / UART

| Code | Rule |
|---|---|
| `ESP_SIM_E010` | `Serial.print` / `write` / `read` called before `Serial.begin()` |

### I2C

| Code | Rule |
|---|---|
| `ESP_SIM_E020` | `Wire.write` outside a `beginTransmission` / `endTransmission` pair |
| `ESP_SIM_E021` | I2C address > 0x7F (7-bit address space violation) |
| `ESP_SIM_E022` | `Wire.beginTransmission` called while a previous transmission was still open |

### Time

| Code | Rule |
|---|---|
| `ESP_SIM_E040` | `delayMicroseconds(>16383)` exceeds the ESP32 hardware maximum (real hw treats high values as ~zero) |

### WiFi / network

| Code | Rule |
|---|---|
| `ESP_SIM_E050` | `WiFi.localIP()` / `RSSI()` / `SSID()` called before WiFi is connected |
| `ESP_SIM_E051` | `HTTPClient::GET` / `POST` called without a prior `HTTPClient::begin(url)` |
| `ESP_SIM_E052` | `PubSubClient::publish` on a disconnected client (message will be silently dropped) |

### Storage / NVS

| Code | Rule |
|---|---|
| `ESP_SIM_E060` | `Preferences::putString` / `clear` etc. called without a prior `begin(namespace)` |
| `ESP_SIM_E061` | NVS namespace > 15 characters (ESP-IDF NVS limit) |

### PWM / LEDC

| Code | Rule |
|---|---|
| `ESP_SIM_E070` | `ledcWrite(channel)` called without a prior `ledcSetup(channel, ...)` |
| `ESP_SIM_E071` | LEDC channel > 7 (ESP32-S3 has channels 0-7 only) |

### BLE

| Code | Rule |
|---|---|
| `ESP_SIM_E080` | `BLEDevice::createServer()` called before `BLEDevice::init()` |
| `ESP_SIM_E081` | `BLEServer::createService()` called before `BLEDevice::init()` |

## Inspecting violations

```cpp
auto& strict = esp32sim::Strict::instance();
strict.count();                       // total violations
strict.count("ESP_SIM_E001");         // count of a specific code
strict.has("ESP_SIM_E001");           // bool
strict.all();                         // const std::vector<Violation>& with timestamp_us
strict.print_report();                // pretty-prints all violations to stdout
```

Each `Violation` carries a `code`, a human `message` (with context like the
specific pin or address), and a `timestamp_us` (virtual time at which it
fired).

## Not yet enforced (deferred to v1.2+)

These rules require source-level analysis or richer ISR-context tracking:

- **`millis()` rollover antipattern** (`if (millis() > t + period)` instead
  of `if (millis() - t > period)`) — needs AST analysis.
- **Heap allocation in ISR context** — needs ISR-context propagation.
- **`Serial.print` / `delay` in ISR callback** — needs ISR-context.
- **Resource leaks** (`Preferences::begin` without matching `end`) — needs
  lifecycle tracking + finalize hook.
- **Return-value-ignored patterns** (`Wire.endTransmission` returns NACK,
  next code proceeds) — needs caller analysis.

## Source authoritative references

The rules are derived from these authoritative sources:

- [ESP32-S3 datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [ESP-IDF Programming Guide — Fatal Errors](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/fatal-errors.html)
- [ESP-IDF GPIO API Reference](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html)
- [arduino-esp32 docs](https://docs.espressif.com/projects/arduino-esp32/en/latest/)
- [Random Nerd ESP32 Pinout reference](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
- [Quantum Leaps DBC for embedded C](https://github.com/QuantumLeaps/DBC-for-embedded-C) — methodology
- [Bertrand Meyer Design by Contract for embedded](https://www.embedded.com/programming-embedded-systems-applying-assertions-and-design-by-contract/) — pattern
