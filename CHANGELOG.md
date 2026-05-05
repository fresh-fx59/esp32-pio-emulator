# Changelog

All notable changes to this project will be documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

(Nothing yet.)

## [1.2.0] — 2026-05-05 — Severity classification + USB-JTAG rule

### Added
- **`Severity` distinction (ERROR vs WARNING).** Each violation carries a
  severity. Errors mean the firmware is definitely broken; warnings mean
  fragile/suboptimal patterns that work at runtime but are worth flagging.
- New API on `esp32sim::Strict`:
  - `errors()` / `warnings()` — filtered violation views.
  - `has_errors()` / `has_warnings()`.
  - `error_count()` / `warning_count()`.
  - `print_report()` now lists errors and warnings in separate sections.
- New rule `ESP_SIM_E007` (WARNING) — `pinMode` on GPIO 19 or 20 (the
  USB-JTAG D-/D+ lines on ESP32-S3) disables USB-JTAG debugging on the
  board. Caught while verifying the user's `iot-yc-water-the-flowers`
  sketch which uses GPIO 19 for the water-level sensor.
- Test pattern in docs: fail tests only on errors; surface warnings as
  info. (`if (s.has_errors()) TEST_FAIL_MESSAGE(...)`).
- 6 new tests covering severity classification + the E007 rule.

### Changed
- Reclassified existing rules:
  - **ERROR (12 rules):** E001, E002, E003, E020, E021, E022, E040, E051,
    E060, E070, E071, E080, E081.
  - **WARNING (8 rules):** E004, E006, E007 (new), E010, E050, E052, E061.
- `Strict::violation` signature now takes an optional `Severity` arg
  (default = ERROR for back-compat with v1.1 callers).

### Notes
- 187 tests passing total (150 v1.0 + 37 strict-mode rules incl.
  severity classification).

## [1.1.0] — 2026-05-05 — Strict mode (autonomous verification)

### Added — autonomous chip-contract verification

The framework's value proposition expands: drop a sketch in, enable strict
mode, the sim runs `setup()` + `loop()` against the chip's contract and
reports violations. **Zero test authoring required.**

- New `esp32sim::Strict` singleton with `enable`/`reset`/`violation`/
  `all`/`has`/`count`/`print_report` API.
- ~20 chip-contract rules encoded as runtime checks inside existing fakes:
  - **GPIO:** E001 (digitalWrite without pinMode), E002 (flash pin use),
    E003 (pin out of range), E004 (analogRead on non-ADC pin), E006
    (strapping pin warning).
  - **Serial:** E010 (use before begin).
  - **I2C:** E020 (write outside transmission), E021 (7-bit addr violation),
    E022 (nested beginTransmission).
  - **Time:** E040 (delayMicroseconds > 16383 hw limit).
  - **WiFi/network:** E050 (read state before begin), E051 (HTTP without
    begin), E052 (MQTT publish on disconnected client).
  - **Storage:** E060 (Preferences use without begin), E061 (namespace too
    long).
  - **PWM:** E070 (ledcWrite without setup), E071 (channel out of range).
  - **BLE:** E080 (createServer before init), E081 (createService before
    init).
- All rules sourced from authoritative documents (ESP32-S3 datasheet,
  ESP-IDF programming guide, arduino-esp32 docs); each violation includes a
  human-readable message and the virtual timestamp at which it fired.
- New docs: `user/reference/strict-mode.md` (rule index + sources),
  `user/how-to/use-strict-mode.md` (the zero-authoring pattern).
- 31 new unit tests for the strict-mode rules; total framework now at
  **181 tests, all green**.
- Sim::reset now also resets stateful Arduino globals (Wire, Wire1, Serial,
  Serial1, Serial2) so strict-mode rule state doesn't leak across tests.

### Notes
- Strict mode is **opt-in** (`Strict::instance().enable()`); existing tests
  unaffected.
- Future v1.2+ will add rules requiring source-level analysis: `millis()`
  rollover antipattern, ISR-context detection, return-value-ignored
  patterns, resource-leak detection.

## [1.0.0] — 2026-05-05 — **🚀 v1.0**

### Added — Tier 4 ships, all four tiers complete

- **NVS / Preferences** — in-memory key/value store with namespaced
  string/uint/int/bool typed access. `Preferences` Arduino class fully
  compatible.
- **Filesystem (LittleFS / SPIFFS-compat)** — in-memory file map; write/
  read/exists/remove/mkdir/list_dir.
- **Deep sleep** — `esp_sleep_enable_timer_wakeup`, `esp_deep_sleep_start`,
  `esp_sleep_get_wakeup_cause`. Tests pre-set the next wake cause; sketch's
  deep-sleep records duration + advances `last_wake`.
- **Cooperative FreeRTOS shim** (per ADR D2 option a) — `xTaskCreate`,
  `xTaskCreatePinnedToCore`, `vTaskDelay`, `xQueueCreate`/`Send`/`Receive`,
  `xSemaphoreCreate{Binary,Counting,Mutex}`/`Take`/`Give`. Tasks don't run
  in parallel; tests drive iterations explicitly.
- **BLE stub** (per ADR D7 option a) — `BLEDevice::init`, `createServer`,
  `createService`, `BLECharacteristic::setValue`/`getValue`/`notify`,
  `BLEAdvertising`. Tests assert via `esp32sim::Ble`.
- **`examples/07-deep-sleep-mqtt/`** — kitchen-sink reference: NVS-stored
  WiFi creds, BLE-provisioning fallback, MQTT publish, 60 s deep sleep,
  boot counter persisting across cycles.
- 3 docs: `test-deep-sleep`, `test-ble-provisioning`, `test-with-littlefs`.

### Notes
- 150 framework tests + 21 example tests across 7 examples = **171 total
  tests, all green** on Ubuntu CI.
- BLE GATT peer fixture deferred to T4.5 (post-v1.0).
- Bluetooth Classic, ULP coprocessor, ESP32 camera support: out of scope
  for v1.0; documented as known gaps.

## [0.4.0] — 2026-05-05

### Added — Tier 3 ships
- **WiFi / WiFiClient / WiFiServer fakes** with state machine, IPAddress,
  `WL_*` status enum.
- **HTTPClient fake** with `begin/GET/POST/getString`. Pre-seeded responses
  via `Sim::Network::seed_http_response(url, ...)`.
- **PubSubClient (MQTT) fake** with `setServer/connect/publish/subscribe/
  loop`. `Sim::mqtt().deliver(topic, payload)` drives incoming messages.
- `examples/06-mqtt-temperature/` — load-bearing T3 acceptance.
- 2 docs: `test-a-wifi-sketch`, `test-mqtt`.
- Path A simplified per [ADR-0004](docs/decisions/0004-pytest-plugin-control-channel.md):
  in-process event recording (not Python mock servers, not real LWIP).

## [0.3.0] — 2026-05-05

### Added
- **Tier 2 — Sensor TDD + pytest-embedded plugin alpha shipped.**
- I2C: two-bus simulator + `Wire` / `Wire1` fakes; attachable `I2CDevice`
  peripherals; full TwoWire API.
- SPI: two-bus simulator + `SPI` fake; CS-routed `SpiDevice` peripherals.
- ADC: per-pin analog values, resolution + attenuation, `analogRead*`.
- PWM/LEDC: 16 channels, `ledcSetup`/`ledcAttachPin`/`ledcWrite`/`analogWrite`.
- Hardware timers: `hw_timer_t` API backed by `VirtualClock`; one-shot +
  autoreload + 4-pool.
- Peripheral fakes: **DS3231 RTC** (Maxim, I2C, BCD time/date + alarm +
  temperature), **BMP280** (Bosch, I2C, calibration + pressure/temp),
  **MCP23017** (Microchip, I2C, 16-bit I/O expander).
- `pytest-pio-emulator` plugin (alpha): subprocess + stdout-only
  `dut.expect()` API; pytest11 entry point. Per [ADR-0004](docs/decisions/0004-pytest-plugin-control-channel.md),
  control channel deferred to T2.5.
- `examples/04-rtc-moisture-logger/` — load-bearing T2 acceptance: same
  sketch driven by both Unity and pytest-embedded.
- `examples/05-pwm-fade/` — LEDC sweep demo.
- 8 docs: `fake-an-i2c-sensor`, `use-pytest-embedded`, `peripherals`,
  `fake-vs-mock-vs-emulate`, `add-a-new-peripheral-fake`,
  `pytest-plugin-architecture`, ADR-0004, plus all Diátaxis subfolder
  READMEs updated.

### Changed
- Repository layout: peripherals moved from `peripherals/<name>/` to
  `src/peripherals/<name>/` and `include/peripherals/` for PIO library
  packaging compatibility (T2 spec drift; consumer ergonomics simpler).
- Umbrella header `<esp32sim/esp32sim.h>` now pulls in all T2 sub-headers
  (i2c, adc, pwm, spi).

### Notes
- 110 framework tests + 5 example tests (3 + 2 in 04 + 05) + 6 plugin
  tests = 121 total tests, all green.
- macOS-13 still deferred from CI per master spec D12.

## [0.2.0] — 2026-05-05

### Added
- **Tier 1 — GPIO TDD shipped.** A user's unmodified Arduino sketch using `pinMode`,
  `digital{Read,Write}`, `Serial`, `millis`, `micros`, `delay`, `attachInterrupt` compiles
  in `[env:native]` against our fakes; Unity tests can drive `setup()` / `loop()` and
  assert on observable behavior. Sub-second feedback. **The TDD-for-ESP32 promise is real
  for any sketch whose hardware footprint is GPIO + UART + timing.**
- Core abstractions: `esp32sim::VirtualClock`, `EventLog`, `PinRegistry`, `UartChannel`.
- Platform fakes: `Arduino.h`, `HardwareSerial.h` (with `Serial`, `Serial1`, `Serial2`
  globals), `attachInterrupt`/`detachInterrupt`/`digitalPinToInterrupt`.
- Public test API: `esp32sim::Sim` (in `<esp32sim_unity/esp32sim.h>`) — `reset`, `runSetup`,
  `runLoop`, `runUntil`, `advanceMs`, `gpio(N)`, `uart(N)`, `events()`.
- Three end-to-end reference examples — each pulls esp32-pio-emulator from GitHub via
  `lib_deps`:
  - `examples/01-blink/` — classic 1Hz blink (3 tests).
  - `examples/02-button-debounce/` — 50ms debouncer (3 tests).
  - `examples/03-serial-echo/` — UART read/write (3 tests).
- Eight new documentation files under `docs/user/` and `docs/dev/`, including the
  framework-vs-ArduinoFake explainer (per ADR-0003) and an interrupt-testing how-to.

### Fixed
- `INPUT_PULLUP` semantics: an external `setLevel(LOW)` now overrides the internal pull-up
  (matches real hardware where an external driver wins). Discovered while bringing up
  `examples/02-button-debounce/`.

### Notes
- 60 unit tests in the framework's own `test/` plus 9 end-to-end tests across the three
  examples — 69 tests total, all green.
- macOS-13 still deferred from CI per master spec D12; T1 introduced no platform-sensitive
  code, so the deferral remains acceptable.

## [0.1.0] — 2026-05-05

### Added
- Tier 0 skeleton: project metadata files, AGENTS.md, CLAUDE.md, Diátaxis docs scaffold,
  architecture overview, dev environment setup tutorial.
- `platformio.ini` with `[env:native]` (gnu++17, Unity test framework).
- `library.json` declaring the project as a PlatformIO library.
- First skeleton Unity test (`test/test_skeleton/test_skeleton.cpp`) — green.
- GitHub Actions CI on Ubuntu 22.04. (macOS-13 in CI deferred — see master spec D12.)
- Pre-commit hooks: clang-format + standard hygiene.
- ADR-0002 (superseded by ADR-0003) recorded in `docs/decisions/` for posterity.

### Notes
- macOS-13 was originally in the CI matrix but was dropped during T0 implementation
  because free GitHub Actions macOS runners queued 40+ minutes without starting. macOS
  remains a supported OS for contributors; CI coverage returns when there is
  platform-sensitive code to test.
- System PIO 4.3.4 on Ubuntu 24/Python 3.12 is broken (Click API incompatibility); the
  dev setup tutorial uses a project-local `.venv/` for PIO 6.x.

## [0.0.0] — 2026-05-05

### Added
- Project conceived. Master design spec and tier specs T0–T4 written and committed.
- ADR-0001 (ESP32-S3 primary target) and ADR-0003 (no ArduinoFake coexistence) accepted.
