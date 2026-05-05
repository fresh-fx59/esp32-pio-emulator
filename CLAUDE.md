# CLAUDE.md — task-handoff context

This file is the **first thing** any agent (Claude Code, Codex, Gemini, human contributor)
should read when picking up work on this repo. It complements `AGENTS.md` (durable
preferences) with the *current* state of work.

## Project mission

Build `esp32-pio-emulator`: a behavioral simulator for ESP32 firmware that runs natively on
a developer's machine. The user's unmodified Arduino sketch compiles against host-side
fakes; Unity tests assert on observable behavior — pin levels, serial output, peripheral
interactions, virtual time. **Strict mode** (v1.1+) enables autonomous chip-contract
enforcement so a sketch can be verified without authoring assertions.

See [`docs/superpowers/specs/2026-05-05-esp32-pio-emulator-master-design.md`](docs/superpowers/specs/2026-05-05-esp32-pio-emulator-master-design.md)
for the full architecture spec.

## Current state

- **Latest release:** **v1.2.0** (2026-05-05).
- **All four tiers shipped** (T0..T4 — capabilities documented in [README's status table](README.md#status)).
- **Strict mode (v1.1) + severity classification (v1.2)** ship the autonomous-verification
  experience: 21 chip-contract rules across GPIO/Serial/I2C/SPI/Time/WiFi/HTTP/MQTT/NVS/
  PWM/BLE, classified as ERROR (must-fix) or WARNING (recommendation).
- **187 tests passing** on Ubuntu CI (150 framework/example + 37 strict-mode rule tests).
- **macOS-13 deferred from CI** per master spec D12; no platform-sensitive code yet
  warrants re-adding it.

## Future work (post-v1.2)

- **T2.5 — pytest plugin control channel** (per [ADR-0004](docs/decisions/0004-pytest-plugin-control-channel.md)):
  Python-side virtual time advancement, peripheral attach, sketch stdin.
- **T3.5 — real-LWIP networking** (Path B): replace event-recording HTTP/MQTT with
  in-process socket fidelity. Capture as new ADR if started.
- **T4.5 — behavioral BLE with GATT peer fixture**.
- **Variant matrix:** S2/C3/C6 first-class beyond ESP32-S3.
- **macOS-13** back in CI when a contributor exhibits a platform-sensitive code path.
- **More peripheral fakes:** SSD1306, MPU6050, BME280, INA219 (deferred from T2).
- **PlatformIO library-registry release** — currently consumed via GitHub URL; the
  `library.json` `name` is reserved for `esp32-pio-emulator`.
- **More strict-mode rules** (v1.3+ candidates): millis() rollover antipattern detection,
  ISR-context tracking, return-value-ignored detection, resource-leak finalize hooks.

## Repository layout

```
esp32-pio-emulator/
├── AGENTS.md, CLAUDE.md, README.md, CHANGELOG.md, LICENSE
├── platformio.ini                 # [env:native] only — we're a library
├── library.json                   # PIO library metadata; native-only
├── include/                       # public headers (PIO convention)
│   ├── Arduino.h                  # the load-bearing fake
│   ├── HardwareSerial.h, Wire.h, SPI.h, WiFi.h, HTTPClient.h,
│   ├── PubSubClient.h, BLEDevice.h, Preferences.h, esp_sleep.h,
│   ├── esp32_hwtimer.h, freertos/{FreeRTOS,task,queue,semphr}.h
│   ├── esp32sim/                  # public sim API
│   │   └── {esp32sim,clock,event_log,gpio,i2c,spi,uart,adc,pwm,
│   │        network,storage,sleep,rtos,ble,strict}.h
│   └── esp32sim_unity/esp32sim.h  # Unity test API
├── src/
│   ├── core/                      # framework-neutral primitives
│   ├── platforms/arduino_esp32/   # Arduino HAL fakes
│   ├── peripherals/               # DS3231, BMP280, MCP23017
│   └── harness/unity/             # ESP32Sim::* implementation
├── harness/pytest_pio_emulator/   # Python pytest plugin (alpha)
├── examples/                      # 7 worked examples (01-blink → 07-deep-sleep-mqtt)
├── test/                          # framework's own self-tests
├── docs/                          # Diátaxis split + specs/plans/decisions
└── .github/workflows/ci.yml       # Ubuntu CI
```

## Workflow

- `AGENTS.md` defines durable preferences (subagent-driven plan execution, TDD default,
  small commits, per-step verify-and-document, spec-drift policy, decide-under-uncertainty,
  critique-once).
- For new tier work: refresh spec → writing-plans skill → execute (subagent-driven by
  default) → verify → commit → push → tag → next.
- Operator authorized sustained autonomous mode — do not prompt between phases. Surface
  only on real blockers or destructive actions.

## Quick verification commands

```bash
.venv/bin/pio test -e native                    # all 187 tests
.venv/bin/pio test -e native --filter test_strict_mode  # the 37 strict-mode tests
.venv/bin/pip install -e harness/pytest_pio_emulator    # install the pytest plugin alpha
```

PlatformIO must be 6.x. The system `pio` on Ubuntu 24+ / Python 3.12 is broken; use the
`.venv/` setup documented in
[`docs/dev/tutorials/setting-up-dev-environment.md`](docs/dev/tutorials/setting-up-dev-environment.md).
