# Architecture overview

A 10-minute prose tour of how `esp32-pio-emulator` is built. For the canonical detailed
spec, read [`../../superpowers/specs/2026-05-05-esp32-pio-emulator-master-design.md`](../../superpowers/specs/2026-05-05-esp32-pio-emulator-master-design.md).

## What it is

A behavioral simulator: the user's ESP32 sketch is compiled *natively* on the host (Linux
or macOS) against drop-in replacements for the Arduino-ESP32 framework's headers. The
sketch runs as a regular host program. The simulator does **not** emulate the Tensilica or
RISC-V CPU; it intercepts the hardware abstraction layer and substitutes behaviorally-faithful
fakes.

This puts us in a different category from QEMU (CPU emulation) and Wokwi (hosted, circuit-
focused, closed-source). We're more comparable to AUnit + EpoxyDuino — but we go further by
making the hardware fakes *behave* (responding to I2C reads, simulating bus transactions)
rather than just stubbing them out.

## The three layers

```
┌──────────────────────────────────────────────────────────┐
│ User's ESP32 sketch (unchanged)                          │
│ digitalWrite(2, HIGH); WiFi.begin(...); Wire.write(...); │
└──────────────────────────────────────────────────────────┘
                       │ compiled against
                       ▼
┌──────────────────────────────────────────────────────────┐
│ platforms/arduino-esp32/ — fake Arduino.h, WiFi.h, ...   │
│ Header-compatible drop-ins, forward to → core/           │
└──────────────────────────────────────────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────────────────┐
│ core/ — VirtualClock, EventLog, PinRegistry, I2CBus, ... │
│ Framework-neutral simulation primitives.                 │
└──────────────────────────────────────────────────────────┘
```

The platform adapter (`platforms/arduino-esp32/`) is where `Wire.beginTransmission(0x76)`
gets translated into core-level "I2C bus 0, address 0x76, start a write transaction." The
core (`core/`) doesn't know about Arduino — it knows about I2C as a protocol. That separation
is what lets a future `platforms/esp-idf/` plug in alongside.

## The two test surfaces

We expose two ways to write tests against the same simulator state:

| Surface | Test layer | When to use |
|---|---|---|
| **Unity in-process** (`harness/unity/`) | unit | "after `setup()`, GPIO 2 is HIGH within 500 ms" |
| **pytest-embedded out-of-process** (`harness/pytest_pio_emulator/`, T2+) | scenario | "device boots → connects WiFi → fetches HTTP → publishes MQTT → recovers from disconnect" |

The pytest-embedded surface plugs into Espressif's existing pytest-embedded ecosystem. The
same test code that drives Wokwi or real hardware drives our sim — that's the leverage.

## Tier roadmap (paraphrased)

T0 (now) — skeleton. T1 — GPIO/Serial/timing. T2 — I2C/SPI/ADC/PWM/timers, plus the pytest
plugin. T3 — WiFi, HTTP, MQTT. T4 — filesystem, NVS, deep-sleep, RTOS, BLE.

Each tier ships standalone with verifiable acceptance against the operator's two real
test-candidate projects (`iot-yc-water-the-flowers` and its mini fork).

## Read next

- The [master design spec](../../superpowers/specs/2026-05-05-esp32-pio-emulator-master-design.md)
  for the canonical version of all of the above.
- The current tier's spec under `docs/superpowers/specs/`.
- The current tier's plan under `docs/superpowers/plans/`.
- ADRs under [`../../decisions/`](../../decisions/) for *why* specific choices were made.
