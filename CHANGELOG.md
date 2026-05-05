# Changelog

All notable changes to this project will be documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

(Nothing yet — Tier 2 (sensor TDD + pytest-embedded plugin) is next.)

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
