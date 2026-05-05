# Tier 2 — Sensor TDD + pytest-embedded plugin

| | |
|---|---|
| **Status** | Draft v0.1 — pending user review |
| **Date** | 2026-05-05 |
| **Parent** | [Master design](2026-05-05-esp32-pio-emulator-master-design.md) |
| **Depends on** | [Tier 1](2026-05-05-tier-1-gpio-tdd-design.md) shipped |
| **Confidence** | Medium-high — detailed but expect to revisit at T2 entry |

## Goal

A developer can test code that talks to **bus-attached sensors** (I2C, SPI), reads analog inputs (ADC), drives PWM outputs (LEDC), reacts to hardware timers, and uses interrupt-driven peripherals. Stateful **fake peripherals** (BMP280, MCP23017 to start) respond like real chips would.

In parallel, the **pytest-embedded service plugin** lands in alpha. The same sketch can now be tested two ways: in-process Unity for tight unit tests, out-of-process pytest-embedded for scenario tests that drive the simulated device from outside.

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

### In — first stateful peripheral fakes (`peripherals/`)

| Fake | Real chip | Why it's first |
|---|---|---|
| `FakeBMP280` | Bosch BMP280 (pressure + temp) | Most common ESP32 sensor in tutorials; covers I2C register-bank pattern. |
| `FakeMCP23017` | Microchip MCP23017 (16-pin I/O expander) | Covers I2C with bidirectional GPIO; common in real projects. |
| `FakeSSD1306` | OLED driver (SSD1306 over I2C) | Covers display semantics; gives users something visual. |
| `FakeMPU6050` | InvenSense MPU6050 (IMU) | Covers DMP-style FIFO + larger register space; lays the pattern for sensor-with-state. |

### In — pytest-embedded plugin (alpha)

The Python harness `harness/pytest_pio_emulator/` packaged as `pytest-pio-emulator`. Implements the pytest-embedded `Dut` interface so existing pytest-embedded users can target `--embedded-services pio_emulator` instead of `wokwi` / `qemu` / `esp` and run the same tests.

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

## Implementation steps

(13 steps; mirroring T1's pattern — each ends with verify + docs in same commit series.)

1. **`core/I2CBus` + tests** against direct C++.
2. **`core/SPIBus` + tests**.
3. **`platforms/arduino-esp32/Wire`** — `TwoWire` + globals `Wire`, `Wire1`.
4. **`platforms/arduino-esp32/SPI`** — `SPIClass` + global `SPI`.
5. **`peripherals/bmp280/FakeBMP280`** — register bank, chip ID, calibration coefficients per datasheet, pressure/temp formulas. Datasheet-conformance tests.
6. **`peripherals/mcp23017/FakeMCP23017`** — register bank, IODIR / GPIO / GPPU / IPOL semantics. Conformance tests.
7. **`peripherals/ssd1306/FakeSSD1306`** — frame buffer, command parser, "I drew this" introspection API.
8. **`peripherals/mpu6050/FakeMPU6050`** — 6-axis register set; can drive simulated motion via test API.
9. **ADC simulator + `analogRead*`**.
10. **PWM/LEDC simulator + `ledc*` / `analogWrite`**.
11. **Hardware timer simulator + `timer*`**.
12. **`harness/pytest_pio_emulator/`** Python plugin — scaffold, `Dut` impl, control protocol C++-side. **This is the big step**; consider a feature branch with multiple commits.
13. **End-to-end examples:**
    - `examples/03-bmp280-logger/` — sketch reads BMP280, logs over Serial. Unity test + pytest-embedded scenario test.
    - `examples/04-mcp23017-button-led/` — expander-driven button + LED.
    - `examples/05-pwm-fade/` — LEDC fade.
14. **Tier 2 sign-off:** docs (tutorials, how-tos for each new capability, control protocol reference), CHANGELOG, README, CLAUDE.md update.

## Verification gate (Tier 2 acceptance)

T2 ships when:

- All four reference examples pass in CI **both ways** (Unity + pytest-embedded).
- `pip install pytest-pio-emulator` works (or local equivalent if not yet on PyPI).
- A user can drop in any of the four fakes by name, attach it, and have a stock Adafruit / SparkFun library against the real chip "just work".
- Coverage of `core/`, `platforms/`, `peripherals/`, `harness/pytest_pio_emulator/` ≥ 80% lines.
- The pytest-embedded plugin is documented in `docs/user/how-to/use-pytest-embedded.md` with side-by-side Unity vs pytest examples.

## Documentation deliverables

- `docs/user/how-to/fake-an-i2c-sensor.md`
- `docs/user/how-to/use-pytest-embedded.md`
- `docs/user/reference/peripherals.md` — table of available fakes + which library they're compatible with.
- `docs/user/reference/sim-control-protocol.md` — for advanced Python users who want to roll their own harness.
- `docs/dev/how-to/add-a-new-peripheral-fake.md` — the playbook for fakes 5+.
- `docs/dev/reference/control-protocol.md` — wire format spec.
- `docs/decisions/0001-pytest-embedded-plugin-architecture.md` — ADR.

## Open questions (T2-specific)

- **Q-T2-1:** Build invocation from the pytest plugin — call `pio run` directly, or use a `compile_commands.json` + cmake fallback? *Default: `pio run` for the v1 alpha. Add cmake fallback only if PlatformIO friction surfaces.*
- **Q-T2-2:** Control protocol: JSON-RPC vs MessagePack vs Cap'n Proto? *Default: JSON-RPC over Unix socket. Trade verbosity for debuggability; revisit if perf becomes an issue.*
- **Q-T2-3:** SPI device count per bus? *Default: support up to 4 simultaneously-attached SPI devices per bus. Document; expand if needed.*
- **Q-T2-4:** Which fake do we make `examples/03-...` use? *Default: BMP280, because Adafruit's BMP280 library is canonical and the datasheet is well-known.*
- **Q-T2-5:** Do we attempt I2C clock-stretching simulation? *Default: no in v1; document as a known gap. Re-evaluate at T3 entry.*
