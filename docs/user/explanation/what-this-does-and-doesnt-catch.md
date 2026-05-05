# What this framework catches — and what it doesn't

`esp32-pio-emulator` is a behavioral simulator, not a cycle-accurate emulator. It catches a
large fraction of firmware bugs, but not all of them. Knowing the gap is part of using it
honestly — the simulator does not replace real-hardware QA.

## What it catches

- **Logic bugs** — off-by-one in a state machine, a stale flag, a wrong calculation.
- **Wrong API usage** — `digitalWrite` before `pinMode`, `Serial.println` without `begin`,
  reading past the end of a buffer.
- **Timing bugs at the millisecond / microsecond scale** — a debouncer that's 1 ms too short,
  a loop that takes 600 ms when it should be 500.
- **Interrupt logic bugs** — wrong edge mode, missed initial state, ISR firing too often.
- **Serial protocol bugs** — wrong byte ordering, missing newlines, malformed payloads.

## What it doesn't catch (yet)

### T1-specific
- **Race conditions arising from real ESP32 dual-core preemption.** The fake runs
  single-threaded; ISRs fire synchronously between sketch operations. Real hardware
  concurrency bugs may slip through. T4 RTOS shim narrows this gap.
- **I2C, SPI, ADC, PWM, hardware timer behavior** — landing in T2.
- **WiFi / HTTPS / MQTT scenarios** — landing in T3.
- **Filesystem / NVS / deep-sleep behavior** — landing in T4.

### Permanent gaps (won't be addressed)
- **Voltage / current / electrical effects.** A pin is HIGH or LOW; we don't model rise
  time, capacitance, ESD, supply ripple, or short-circuits.
- **Compiler / toolchain differences.** Code is compiled with the host compiler (gcc/clang),
  not `xtensa-esp32-elf-gcc`. Architecture-specific bugs (e.g., misaligned access on
  Xtensa, signed-int arithmetic that wraps differently) won't reproduce.
- **Real radio behavior.** No RF interference, range, scan accuracy, or interference
  with Bluetooth.
- **Real timing precision.** Sub-microsecond timing is not modeled.
- **Bootloader, partition table, OTA verify** — these are real-hardware properties, not
  simulator concerns.

## Implication for your test pipeline

Use the simulator as your fast TDD loop and CI gate. Use real hardware as the final
sign-off before shipping firmware. They complement each other. If you've never tested on
real hardware after using the simulator, you haven't fully validated the firmware.
