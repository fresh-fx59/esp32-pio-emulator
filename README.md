# esp32-pio-emulator

> Status: **Tier 2 starting** (T0 ✓, T1 ✓ shipped 2026-05-05 v0.2.0) · License: MIT · ESP32-S3 primary target · gnu++17

A behavioral simulator for ESP32 firmware that runs natively on a developer's machine.
Compile your unmodified Arduino sketch against host-side fakes of the ESP32 hardware
abstraction layer; run Unity tests; assert on what your code makes the chip do — without
flashing a board.

**Why:** the ESP32 testing landscape has fast-but-shallow (manual mocks), slow-but-broad
(real hardware, Wokwi, QEMU), or fast-but-closed (Wokwi). This project targets the
**fast + broad + open-source + locally-runnable** quadrant.

## Status

This repository is in active early development. See [`docs/superpowers/specs/`](docs/superpowers/specs/)
for design specs and [`docs/decisions/`](docs/decisions/) for ADRs.

| Tier | Capability | Status |
|---|---|---|
| T0 | Skeleton | ✓ shipped 2026-05-05 (v0.1.0) |
| T1 | GPIO TDD | ✓ shipped 2026-05-05 (v0.2.0) |
| T2 | Sensor TDD + pytest-embedded plugin | 🚧 next |
| T3 | Networked ESP32 (WiFi, HTTP, MQTT) | ⏸ |
| T4 | Full chip (filesystem, NVS, deep-sleep, BLE, RTOS) | ⏸ |

## Getting started

(After T1 ships.) Add to your PlatformIO project's `[env:native]`:

```ini
[env:native]
platform = native
test_framework = unity
lib_deps =
    https://github.com/fresh-fx59/esp32-pio-emulator
build_flags = -std=gnu++17
```

## Documentation

- [Specs](docs/superpowers/specs/) — design rationale and tier roadmap
- [Decisions](docs/decisions/) — architecture decision records
- [User docs](docs/user/) — how to write tests against the simulator
- [Developer docs](docs/dev/) — how to contribute to the framework

## License

MIT — see [LICENSE](LICENSE).
