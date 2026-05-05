# Tier 2 — Sensor TDD + pytest-embedded plugin

| | |
|---|---|
| **Status** | v0.2 — refreshed at T2 entry; implementation-ready |
| **Date** | 2026-05-05 (v0.1 → v0.2) |
| **Parent** | [Master design v0.3](2026-05-05-esp32-pio-emulator-master-design.md) |
| **Depends on** | [Tier 1](2026-05-05-tier-1-gpio-tdd-design.md) shipped (v0.2.0) |
| **Confidence** | Medium — implementation-ready; expect spec drift during T2 (especially around the pytest-embedded plugin) |

## Goal

A developer can test code that talks to **bus-attached sensors** (I2C, SPI), reads analog inputs (ADC), drives PWM outputs (LEDC), reacts to hardware timers, and uses interrupt-driven peripherals. Stateful **fake peripherals** (BMP280, MCP23017 to start) respond like real chips would.

In parallel, the **pytest-embedded service plugin** lands in alpha. The same sketch can now be tested two ways: in-process Unity for tight unit tests, out-of-process pytest-embedded for scenario tests that drive the simulated device from outside.

## Changelog

| Date | Version | Change |
|---|---|---|
| 2026-05-05 | v0.1 | Initial draft during brainstorming. |
| 2026-05-05 | v0.2 | T2-entry refresh. (a) Peripheral fake set narrowed to DS3231 + BMP280 + MCP23017; SSD1306 + MPU6050 deferred to T2.5. (b) Reference example switched from generic BMP280 to RTC+moisture (matches user's test-candidate projects). (c) pytest-embedded plugin scope reduced to alpha-minimum (subprocess + dut.expect via stdout); richer control deferred. (d) Q-T2-1..5 resolved per AGENTS.md decide-under-uncertainty. (e) Spec-drift expectations section added. |

## Scope

### In — APIs that work behaviorally

| Arduino / framework API | Sim behavior |
|---|---|
| `Wire`, `TwoWire` | I2C as a real bus simulator. `beginTransmission`, `endTransmission`, `requestFrom`, `write`, `read` route to `core/I2CBus`, which routes to attached `peripherals/`. |
| `SPI`, `SPIClass` | SPI bus simulator with CS routing. `beginTransaction`, `transfer`, `transfer16`, `endTransaction`. |
| `analogRead(pin)`, `analogReadResolution`, `analogSetAttenuation` | ADC simulator. Tests can drive analog values into pins. |
| `ledcSetup`, `ledcAttachPin`, `ledcWrite`, `analogWrite` | PWM simulator. Tests can read duty cycle / frequency. |
| `attachInterrupt` (extended) | Now also fires on hardware events: SPI/I2C transactions, timer events, ADC threshold. |
| `hw_timer_t`, `timerBegin`, `timerAttachInterrupt`, `timerAlarm`, `timerStart` | Hardware timers backed by `VirtualClock`. |

### In — peripheral fakes (`peripherals/`) — **prioritized for the test-candidate projects**

The reference projects (`iot-yc-water-the-flowers*`) actually use **DS3231 RTC** (I2C) and analog moisture sensors (ADC) — not BMP280. T2 v0.2 prioritizes accordingly.

| Fake | Real chip | Why |
|---|---|---|
| `FakeDS3231` | Maxim DS3231 (precision I2C RTC + 32 KB EEPROM) | Used by the test-candidate projects. Covers I2C register-bank + time/date conversion + alarm interrupt pattern. **MUST-HAVE for T2 acceptance.** |
| `FakeBMP280` | Bosch BMP280 (pressure + temp) | Iconic Arduino tutorial example. Covers I2C register-bank + calibration-coefficient + computed-output pattern. Adafruit_BMP280 library compatible. **MUST-HAVE — the canonical "first sensor" sample.** |
| `FakeMCP23017` | Microchip MCP23017 (16-pin I/O expander) | I2C with bidirectional GPIO; teaches the pattern for "I2C peripheral that exposes pin-level state." **SHOULD-HAVE.** |

**Deferred to T2.5 (post-T2 if no demand):** SSD1306 (display — complex graphics modeling, no user demand), MPU6050 (IMU FIFO — complex, no user demand). Pattern is documented in `dev/how-to/add-a-new-peripheral-fake.md` so a contributor or T2.5 can add them readily.

### In — pytest-embedded plugin (alpha — minimal viable surface)

The Python harness `harness/pytest_pio_emulator/` packaged as `pytest-pio-emulator`. **Alpha scope is deliberately minimal:** prove that the architecture works (Python launches a sim binary as a subprocess, reads its serial output, can be used in pytest fixtures), defer advanced fixtures (control-socket-driven time advancement, fixture-attached fake peripherals from Python) to T2.1 / T2.5.

Alpha = enough to write `dut.expect("expected serial line")` against a sim binary. NOT alpha: `dut.sim.advance_time_ms(...)` or `dut.sim.attach_i2c(0x68, FakePeripheral())`.

```python
# example: a scenario test that runs against the sim
def test_blink_scenario(dut):
    dut.expect("LED on")
    dut.expect("LED off")
    assert dut.sim.gpio(2).level == 1  # sim-specific extension
    dut.sim.advance_time_ms(500)
    dut.expect("LED off")
```

The plugin:
- Builds the user's project for `native` (`pio run -e native`).
- Spawns the test binary as a subprocess.
- Connects to its stdout (= `Serial`) and a control socket (for time advancement, peripheral attach, gpio injection from Python).
- Provides a `dut` fixture compatible with pytest-embedded.
- Provides sim-specific extensions on `dut.sim` for time/peripheral/gpio control.

### Out (still deferred)
- WiFi, networking, HTTP, MQTT — Tier 3.
- Filesystem, NVS — Tier 4.
- RTOS / dual-core — Tier 4.
- BLE / BT — Tier 4.

## Architecture additions

### I2C bus simulator (`core/I2CBus`)

```cpp
class I2CBus {
public:
  void attach(uint8_t addr, std::shared_ptr<I2CDevice>);
  void detach(uint8_t addr);
  Result write(uint8_t addr, const uint8_t* data, size_t len);
  Result read(uint8_t addr, uint8_t* buf, size_t len);
  Result writeRead(uint8_t addr, const uint8_t* w, size_t wn,
                                  uint8_t* r,       size_t rn);
};

class I2CDevice {
public:
  virtual Result on_write(const uint8_t* data, size_t len) = 0;
  virtual Result on_read(uint8_t* buf, size_t len) = 0;
};
```

Two I2C buses by default (`Wire`, `Wire1`) — matches ESP32.

### SPI bus simulator

Symmetric design: `SPIBus`, `SPIDevice`, attach by CS pin. Three SPI buses on ESP32 (HSPI, VSPI, FSPI on newer variants — initially just two for v1).

### Peripheral fake structure

Every fake under `peripherals/<name>/` has:

```
peripherals/bmp280/
├── include/peripherals/FakeBMP280.h
├── src/FakeBMP280.cpp
├── test/test_FakeBMP280.cpp           # datasheet-conformance tests
└── README.md                          # what it simulates, what it doesn't
```

Each fake is **datasheet-conformance tested** independent of any sketch — i.e., we directly poke the fake with I2C transactions and assert the registers behave per the datasheet. This means a sketch using a real `Adafruit_BMP280` library can talk to our `FakeBMP280` without modification.

### pytest-embedded plugin design

The plugin entry point is `pytest_pio_emulator/plugin.py`:

```python
class PioEmulatorDut(pytest_embedded.Dut):
    def __init__(self, app, ...): ...
    def write(self, data): ...           # → subprocess stdin
    def expect(self, pattern, ...): ...  # ← subprocess stdout
    @property
    def sim(self) -> SimController: ...

class SimController:
    """Talks to the test binary's control socket."""
    def advance_time_ms(self, ms): ...
    def gpio(self, pin) -> GpioRef: ...
    def attach_i2c(self, addr, fake): ...
    def reset(self): ...
```

The control protocol between Python harness and C++ binary is a simple length-prefixed JSON-RPC over a Unix socket. Documented in `docs/dev/reference/control-protocol.md`.

## Implementation steps (v0.2 — focused)

12 tasks; each ends with verify + docs in same commit series per AGENTS.md.

1. **`include/esp32sim/i2c.h` + `src/core/i2c_bus.cpp` — I2CBus core.** Two buses (Wire, Wire1). `attach(addr, device)`, `write`, `read`, `write_read` routes to attached `I2CDevice`s.
2. **`include/Wire.h` + `src/platforms/arduino_esp32/wire.cpp`** — `TwoWire` class + globals `Wire`, `Wire1`. `beginTransmission`, `write`, `endTransmission`, `requestFrom`, `available`, `read`, `peek`. Forwards to I2CBus.
3. **`include/esp32sim/adc.h` + `src/core/adc.cpp`** — ADC simulator. Tests drive analog values into pins via `Sim::adc(pin).setValue(v)`. Resolution + attenuation handling.
4. **`include/Arduino.h` ADC funcs + impl** — `analogRead`, `analogReadResolution`, `analogSetAttenuation`. Forwards to core.
5. **`include/esp32sim/pwm.h` + `src/core/pwm.cpp`** — PWM simulator. Tests query duty cycle + frequency via `Sim::pwm(channel).dutyCycle()`.
6. **`include/Arduino.h` PWM funcs + impl** — `ledcSetup`, `ledcAttachPin`, `ledcWrite`, `analogWrite`. Forwards to core.
7. **`include/esp32sim/spi.h` + `src/core/spi_bus.cpp`** — SPI bus simulator with CS routing.
8. **`include/SPI.h` + `src/platforms/arduino_esp32/spi.cpp`** — `SPIClass` + global `SPI`. `beginTransaction`, `transfer`, `transfer16`, `endTransaction`.
9. **Hardware timers** — `hw_timer_t`, `timerBegin`, `timerAttachInterrupt`, `timerAlarmWrite`, `timerAlarmEnable` backed by `VirtualClock::schedule_at`.
10. **`peripherals/ds3231/FakeDS3231`** — register bank, time/date encoding (BCD), alarm registers. Datasheet-conformance tests.
11. **`peripherals/bmp280/FakeBMP280`** — register bank, chip ID 0x58, calibration coefficients per datasheet, pressure/temp formulas. Adafruit_BMP280-library-compatible.
12. **`peripherals/mcp23017/FakeMCP23017`** — IODIR/GPIO/GPPU/IPOL registers, bidirectional pin-level state.
13. **`harness/pytest_pio_emulator/` Python plugin (alpha)** — Python package, subprocess Dut wrapping a `pio test` binary, `dut.expect()` against stdout. Documented in `dev/reference/pytest-plugin-architecture.md`.
14. **End-to-end examples:**
    - `examples/04-rtc-moisture-logger/` — sketch reads DS3231 + ADC, logs over Serial. Unity test + pytest-embedded scenario test (the load-bearing T2 acceptance proof).
    - `examples/05-pwm-fade/` — LEDC fade demo.
15. **Documentation** — peripheral.md reference, fake-an-i2c-sensor.md how-to, use-pytest-embedded.md how-to, fake-vs-mock-vs-emulate.md explanation, control-protocol-architecture.md dev reference, plus `add-a-new-peripheral-fake.md` so contributors can add SSD1306/MPU6050 later.
16. **Tier 2 sign-off** — CHANGELOG 0.2.0 → 0.3.0, README + CLAUDE.md updates, tag `v0.3.0`.

## Verification gate (Tier 2 acceptance — v0.2 sharpened)

T2 ships when:

- The two reference examples (04-rtc-moisture-logger, 05-pwm-fade) pass in CI.
- `examples/04-rtc-moisture-logger/` runs **both** under `pio test -e native` (Unity) AND under `pytest` driving the same binary (pytest-embedded plugin alpha) — same sketch, two test surfaces. **This is the load-bearing acceptance proof.**
- DS3231, BMP280, MCP23017 fakes pass datasheet-conformance tests in `test/test_peripherals_*/`.
- A scratch consumer project that adds esp32-pio-emulator via lib_deps and uses `Wire` to read a fake DS3231 works end-to-end (mirrors T0's consumer-side smoke pattern).
- `pip install -e harness/pytest_pio_emulator/` works locally; `pytest examples/04-rtc-moisture-logger/` runs the Python scenario test green.
- Documentation: every new capability has the four Diátaxis quadrants populated for it.

## Spec drift expectations

I expect drift in T2 around three areas — flagged so we update the spec first per AGENTS.md:

1. **The pytest-embedded plugin's exact integration pattern** — whether to use a control socket for stdin/stdout-based commands, a Unix domain socket for richer control, or just stdout-only. Decide at task 13 entry; record as `docs/decisions/0004-pytest-plugin-control-channel.md`.
2. **DS3231 fake fidelity depth** — full register set (90 registers including alarm/control) vs minimum-viable (current time + temperature). Pick MV first; expand if user code exercises more.
3. **MCP23017 vs separate IODIRA/IODIRB ↔ unified addressing modes (BANK=0 vs BANK=1)** — datasheet has both modes; default is BANK=0 (registers paired). Stick with the default; document the gap.

## Documentation deliverables

- `docs/user/how-to/fake-an-i2c-sensor.md`
- `docs/user/how-to/use-pytest-embedded.md`
- `docs/user/reference/peripherals.md` — table of available fakes + which library they're compatible with.
- `docs/user/reference/sim-control-protocol.md` — for advanced Python users who want to roll their own harness.
- `docs/dev/how-to/add-a-new-peripheral-fake.md` — the playbook for fakes 5+.
- `docs/dev/reference/control-protocol.md` — wire format spec.
- `docs/decisions/0001-pytest-embedded-plugin-architecture.md` — ADR.

## Resolved decisions (T2)

(All Q-T2-* resolved per AGENTS.md decide-under-uncertainty in autonomous mode.)

- **Q-T2-1 → Resolved:** pytest plugin invokes `pio test -e native` directly (not `pio run`). The test binary IS what we want to drive. cmake fallback deferred.
- **Q-T2-2 → Resolved:** No separate control protocol in T2 alpha. Stdin/stdout-only — sufficient for `dut.expect()` workflows. Richer control (time advancement from Python, peripheral attach from Python) deferred to T2.5 with ADR-0004.
- **Q-T2-3 → Resolved:** SPI supports up to 4 attached devices per bus. Documented.
- **Q-T2-4 → Resolved:** Reference example uses **DS3231 + ADC** (matches user's test-candidate projects), not BMP280 alone. BMP280 fake still ships, used in unit tests.
- **Q-T2-5 → Resolved:** No I2C clock-stretching simulation. Documented as a known gap in `what-this-does-and-doesnt-catch.md`.
