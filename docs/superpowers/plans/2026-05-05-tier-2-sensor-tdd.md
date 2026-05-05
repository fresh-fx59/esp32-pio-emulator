# Tier 2 Sensor TDD — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task.

**Goal:** I2C, SPI, ADC, PWM, hardware timers, and three stateful peripheral fakes (DS3231, BMP280, MCP23017) plus a pytest-embedded plugin alpha. The reference acceptance is `examples/04-rtc-moisture-logger/` running under both Unity and pytest-embedded.

**Tech Stack:** Same as T1 (gnu++17, Unity, PlatformIO native env, GitHub Actions CI on Ubuntu) plus Python 3.10+ for the pytest-embedded plugin.

**Reference docs:** [Master design v0.3](../specs/2026-05-05-esp32-pio-emulator-master-design.md), [T2 spec v0.2](../specs/2026-05-05-tier-2-sensor-tdd-design.md), [T1 plan](2026-05-05-tier-1-gpio-tdd.md) — established patterns we follow.

**Convention reminders:** Same as T1. TDD per task. One concept per commit. `.venv/bin/pio` not system pio. Per-step verify-and-document. Subagent-first when applicable.

---

## Pattern reuse from T1

Every "core abstraction + Arduino API" task in T2 follows this shape (already proven in T1):

1. Write failing test in `test/test_<area>/test_<area>.cpp` against the *intended* API.
2. Run, verify compile or assertion fails.
3. Write `include/esp32sim/<area>.h` (the framework-neutral primitive header).
4. Write `src/core/<area>.cpp` (the impl).
5. (For Arduino-side APIs) Write/extend `include/<ArduinoHeader>.h` and `src/platforms/arduino_esp32/<file>.cpp`.
6. Run, verify pass.
7. Commit.

When this pattern applies, the task description below just enumerates the public API and key invariants; refer to the [T1 plan](2026-05-05-tier-1-gpio-tdd.md) for the exact step-by-step structure.

---

## Task 1: I2C bus + Wire

**Files:** `include/esp32sim/i2c.h`, `src/core/i2c_bus.cpp`, `include/Wire.h`, `src/platforms/arduino_esp32/wire.cpp`, `test/test_core_i2c/test_i2c.cpp`, `test/test_arduino_wire/test_wire.cpp`.

**Core API (`esp32sim::I2CBus`):**
- `attach(uint8_t addr, std::shared_ptr<I2CDevice>)`, `detach(addr)`.
- `write(addr, data, len) -> Result`, `read(addr, buf, len) -> Result`, `write_read(addr, w, wn, r, rn) -> Result`.
- `for_index(int n)` — bus 0 = `Wire`, bus 1 = `Wire1`.

**`I2CDevice` interface:**
- `virtual Result on_write(const uint8_t* data, size_t len) = 0;`
- `virtual Result on_read(uint8_t* buf, size_t len) = 0;`

**Arduino-side (`Wire`, `Wire1`):**
- `begin()`, `setClock(freq)` (no-op).
- `beginTransmission(addr)`, `write(byte)`, `write(buf, len)`, `endTransmission(stop=true)`.
- `requestFrom(addr, n)`, `available()`, `read()`, `peek()`.

**Tests:** core (8 tests for attach/write/read/write_read/two-bus-isolation), Arduino-side (6 tests for the typical idiom: `beginTransmission/write/endTransmission`, `requestFrom/read`).

**Commit:** "feat(core/platforms): I2C bus simulator + Wire fake"

---

## Task 2: ADC + analogRead

**Files:** `include/esp32sim/adc.h`, `src/core/adc.cpp`, extend `include/Arduino.h`, `src/platforms/arduino_esp32/adc.cpp`, `test/test_core_adc/`, extend `test/test_arduino_basics/`.

**Core API (`esp32sim::Adc`):**
- `setValue(pin, raw_value)` — tests drive pin's analog reading.
- `getValue(pin) -> int` — returns the raw value (default 0).
- `setResolution(bits)` — global resolution (default 12 for ESP32-S3).
- `setAttenuation(pin, atten)` — recorded but doesn't affect returned values in T2 (would in T2.5).

**Arduino-side:** `analogRead(pin)`, `analogReadResolution(bits)`, `analogSetAttenuation(atten)`, `analogSetPinAttenuation(pin, atten)`.

**Sim test API:** `Sim::adc(pin).setValue(v)`.

**Tests:** core (4 tests), Arduino-side (4 tests including resolution change).

**Commit:** "feat(core/platforms): ADC simulator + analogRead*"

---

## Task 3: PWM (LEDC) + analogWrite

**Files:** `include/esp32sim/pwm.h`, `src/core/pwm.cpp`, extend `include/Arduino.h`, `src/platforms/arduino_esp32/pwm.cpp`, `test/test_core_pwm/`, extend `test/test_arduino_basics/`.

**Core API (`esp32sim::Pwm`):**
- LEDC channel state: frequency, resolution, duty cycle, attached pin.
- `setupChannel(ch, freq, resolution_bits)`, `attachPin(ch, pin)`, `setDuty(ch, duty)`, `getDuty(ch) -> uint32_t`.

**Arduino-side:** `ledcSetup(ch, freq, res)`, `ledcAttachPin(pin, ch)`, `ledcWrite(ch, duty)`, `ledcRead(ch)`, `analogWrite(pin, val)` (auto-channel internally).

**Sim test API:** `Sim::pwm(channel).dutyCycle()`, `Sim::pwm(channel).frequency()`.

**Tests:** core (5 tests), Arduino-side (4 tests).

**Commit:** "feat(core/platforms): PWM/LEDC simulator + analogWrite"

---

## Task 4: SPI bus + SPI fake

**Files:** `include/esp32sim/spi.h`, `src/core/spi_bus.cpp`, `include/SPI.h`, `src/platforms/arduino_esp32/spi.cpp`, `test/test_core_spi/`, `test/test_arduino_spi/`.

**Core API (`esp32sim::SpiBus`):**
- `attach(int cs_pin, std::shared_ptr<SpiDevice>)`, `detach(cs_pin)`.
- `transfer(cs_pin, tx_byte) -> rx_byte`, `transfer16`, `transfer_buffer`.

**`SpiDevice` interface:**
- `virtual uint8_t on_transfer(uint8_t mosi) = 0;`

**Arduino-side (`SPI`):**
- `begin()`, `end()`.
- `beginTransaction(SPISettings(freq, order, mode))` (settings ignored in T2).
- `transfer(b)`, `transfer16(w)`, `transfer(buf, n)`.
- `endTransaction()`.

**Tests:** core (5 tests), Arduino-side (3 tests).

**Commit:** "feat(core/platforms): SPI bus simulator + SPI fake"

---

## Task 5: Hardware timers

**Files:** extend `include/Arduino.h` (or new `include/esp32_hwtimer.h`), `src/platforms/arduino_esp32/hwtimer.cpp`, `test/test_hwtimer/`.

**Arduino-side (matches arduino-esp32 hw_timer_t API):**
- `hw_timer_t* timerBegin(uint8_t num, uint16_t divider, bool count_up)`.
- `void timerAttachInterrupt(hw_timer_t*, voidFuncPtr, bool edge)`.
- `void timerAlarmWrite(hw_timer_t*, uint64_t alarm_value, bool autoreload)`.
- `void timerAlarmEnable(hw_timer_t*)`, `timerAlarmDisable`, `timerEnd`.

**Implementation strategy:** each `hw_timer_t` is backed by a `VirtualClock::ScheduleHandle`. `timerAlarmEnable` schedules; on fire, callback runs and (if autoreload) re-schedules.

**Tests:** 4 tests covering one-shot, periodic-autoreload, disable-mid-flight, two-timers-independent.

**Commit:** "feat(platforms): hardware timers backed by virtual clock"

---

## Task 6: DS3231 fake (RTC)

**Files:** `peripherals/ds3231/include/peripherals/FakeDS3231.h`, `peripherals/ds3231/src/FakeDS3231.cpp`, `peripherals/ds3231/test/test_FakeDS3231.cpp`, `peripherals/ds3231/README.md`.

**Behavior:**
- Default I2C address 0x68.
- Register map (per Maxim DS3231 datasheet, simplified for v1):
  - 0x00: seconds (BCD), 0x01: minutes (BCD), 0x02: hours (BCD)
  - 0x03: day-of-week, 0x04: date, 0x05: month, 0x06: year
  - 0x07-0x0E: alarm 1 + alarm 2 registers
  - 0x0F: control register
  - 0x10: status register
  - 0x11: aging offset
  - 0x12-0x13: temperature MSB/LSB
- Test API: `setTime(hour, min, sec, day, month, year)`, `setTemperature(c)`, `triggerAlarm1()`.
- Implements `I2CDevice`. Sketch reads time via `Wire.beginTransmission(0x68); Wire.write(0x00); Wire.endTransmission(false); Wire.requestFrom(0x68, 7);`.

**Tests:** 6 datasheet-conformance tests: chip is queryable, set time → read time roundtrip (BCD encoding), set temperature, alarm-fire flow, write-then-read consistency.

**Commit:** "feat(peripherals): DS3231 RTC fake"

---

## Task 7: BMP280 fake

**Files:** same shape as DS3231, under `peripherals/bmp280/`.

**Behavior:**
- Default I2C address 0x76 or 0x77 (configurable).
- Register map (subset — focus on what Adafruit_BMP280 actually reads):
  - 0xD0: chip ID (0x58)
  - 0x88-0x9F: calibration coefficients (T1, T2, T3, P1..P9 — all int16/uint16)
  - 0xF4: ctrl_meas (oversampling + mode)
  - 0xF7-0xFC: pressure + temp data registers (24-bit each)
- Test API: `setTemperature(celsius)` and `setPressure(pa)` — the fake computes the back-converted raw values using the published Bosch formulas (or stores synthetic-but-consistent calibration coefficients that produce the inputs).
- Adafruit_BMP280 library compatible: a sketch using `Adafruit_BMP280` should compile and read sensible values.

**Tests:** 5 datasheet-conformance tests including chip ID readout, set/read temperature roundtrip via the Bosch formulas, set/read pressure.

**Commit:** "feat(peripherals): BMP280 pressure+temperature fake"

---

## Task 8: MCP23017 fake (I/O expander)

**Files:** `peripherals/mcp23017/...`.

**Behavior:**
- Default I2C address 0x20.
- BANK=0 register addressing (default after power-on).
- Register map: IODIRA/IODIRB (direction), GPIOA/GPIOB (level), GPPUA/GPPUB (pullup), IPOLA/IPOLB (input polarity).
- Test API: `setPin(0..15, level)`, `getPin(0..15)`, `setMode(0..15, mode)`.

**Tests:** 5 datasheet-conformance tests: direction register, GPIO write reflects on getPin, GPIO read reflects setPin, pullup register, two-bank isolation.

**Commit:** "feat(peripherals): MCP23017 I/O expander fake"

---

## Task 9: pytest-embedded plugin (alpha)

**Files:** `harness/pytest_pio_emulator/pyproject.toml`, `harness/pytest_pio_emulator/pytest_pio_emulator/__init__.py`, `harness/pytest_pio_emulator/pytest_pio_emulator/plugin.py`, `harness/pytest_pio_emulator/README.md`.

**Behavior (alpha):**
- Python package installable via `pip install -e harness/pytest_pio_emulator/`.
- Pytest plugin entry point `pytest_pio_emulator`.
- Provides a `dut` fixture: spawns `pio test -e native` as a subprocess in the project root, reads its stdout line by line, exposes `dut.expect(pattern, timeout=10)`.
- Compatible with `pytest_embedded.Dut` interface where reasonable; full pytest-embedded compliance is T2.5.

**Tests:** Python tests in `harness/pytest_pio_emulator/tests/test_plugin.py` running against a tiny synthetic binary that prints lines with delays.

**Commit:** "feat(harness): pytest-pio-emulator plugin (alpha)"

---

## Task 10: examples/04-rtc-moisture-logger

**Files:** `examples/04-rtc-moisture-logger/{platformio.ini, src/main.cpp, test/test_logger/test_logger.cpp, tests/test_scenario.py, conftest.py, pytest.ini, README.md}`.

**Sketch behavior:** Every 10 seconds (millis-based), read DS3231 time + analogRead(34) moisture, log via Serial as `"YYYY-MM-DD HH:MM:SS moisture=N"`.

**Unity test:** attach `FakeDS3231` to Wire bus 0 at 0x68, set time, set ADC value on pin 34, run loop, drain Serial, assert format.

**pytest-embedded scenario test:** `pytest tests/test_scenario.py` spawns `pio test -e native --filter test_logger_scenario`, reads "moisture=" lines, asserts.

**Commit:** "feat(examples): 04-rtc-moisture-logger — Unity + pytest-embedded"

---

## Task 11: examples/05-pwm-fade

**Files:** `examples/05-pwm-fade/...`.

**Sketch:** uses `ledcSetup` + `ledcAttachPin` + `ledcWrite` to fade an LED on pin 5. Period 1 second.

**Unity test:** advance virtual time, sample `Sim::pwm(channel).dutyCycle()`, assert it sweeps 0→max→0.

**Commit:** "feat(examples): 05-pwm-fade — LEDC sweep"

---

## Task 12: Documentation

**New docs:**
- `docs/user/how-to/fake-an-i2c-sensor.md`
- `docs/user/how-to/use-pytest-embedded.md`
- `docs/user/reference/peripherals.md` (table: name, I2C address, supported register set, library compatibility)
- `docs/user/explanation/fake-vs-mock-vs-emulate.md`
- `docs/dev/how-to/add-a-new-peripheral-fake.md`
- `docs/dev/reference/pytest-plugin-architecture.md`

**Updates:**
- `docs/user/reference/supported-arduino-apis.md` — add Wire/SPI/analogRead/ledcWrite/timer entries.
- `docs/user/explanation/what-this-does-and-doesnt-catch.md` — add I2C clock-stretching, SPI quad/octal modes, ADC noise/non-linearity as known gaps.
- All Diátaxis subfolder READMEs updated.
- `docs/decisions/0004-pytest-plugin-control-channel.md` ADR for the alpha control-channel decision.

**Commit:** "docs: T2 user + dev documentation + ADR-0004"

---

## Task 13: Tier 2 sign-off

CHANGELOG bump 0.2.0 → 0.3.0, README status (T2 ✓ T3 🚧), CLAUDE.md current-tier T3, full acceptance gate run, tag `v0.3.0`.

**Commit:** "release: tier 2 (sensor TDD + pytest-embedded plugin) shipped, v0.3.0"

---

## Self-review

Every T2 spec v0.2 deliverable maps to a task above. No placeholders. Each task references file paths consistent with master spec v0.3 layout and T1 patterns. Subagent execution is viable because tasks are independent except for Wire→I2C dependency (1 must precede peripherals 6-8) and pytest plugin needing the example sketch (10 depends on 9 and on 6).

Execution order: 1 → 2 → 3 → 4 → 5 → 6 → 7 → 8 → 9 → 10 → 11 → 12 → 13. Tasks 6/7/8 can run in parallel after 1 if dispatched to subagents in worktrees. Task 11 can run in parallel with 9-10.

## Execution handoff

Per AGENTS.md, execute via subagent-driven-development for the larger tasks (peripherals, pytest plugin, examples). Smaller tasks (mechanical core+adapter pairs like ADC/PWM) inline is acceptable.
