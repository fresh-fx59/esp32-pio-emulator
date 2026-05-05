# How to fake an I2C sensor

You want to test code that talks to an I2C peripheral (a sensor, RTC, expander)
without flashing real hardware. There are two paths: use an existing peripheral
fake, or write your own.

## Path 1: use an existing fake

`esp32-pio-emulator` ships fakes for these chips out of the box (T2):

| Chip | Default address | Header | Test API |
|---|---|---|---|
| Maxim DS3231 RTC | 0x68 | `<peripherals/FakeDS3231.h>` | `setTime(h,m,s)`, `setDate(d,m,y)`, `setTemperature(c)` |
| Bosch BMP280 | 0x76 (or 0x77) | `<peripherals/FakeBMP280.h>` | `setTemperature(c)`, `setPressure(pa)` |
| Microchip MCP23017 | 0x20 | `<peripherals/FakeMCP23017.h>` | `setPin(0..15, level)`, `setMode(pin, input)` |

Pattern:

```cpp
#include <Wire.h>
#include <peripherals/FakeDS3231.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void test_my_sketch_reads_rtc(void) {
    esp32sim::Sim::reset();
    esp32sim::I2CBus::reset_all();

    auto rtc = std::make_shared<esp32sim::peripherals::FakeDS3231>();
    rtc->setTime(15, 30, 0);
    esp32sim::I2CBus::for_index(0).attach(0x68, rtc);

    // Now run your sketch
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();

    // Assert on what your sketch wrote to Serial / pins / etc.
}
```

## Path 2: write your own fake

Implement `esp32sim::I2CDevice` for any chip we don't ship a fake of. Two
methods:

```cpp
class FakeMyChip : public esp32sim::I2CDevice {
public:
    static constexpr uint8_t DEFAULT_ADDR = 0x42;

    esp32sim::I2CResult on_write(const uint8_t* data, size_t len) override {
        // First byte usually selects the register; subsequent bytes are
        // register writes. See FakeDS3231 for the canonical pattern.
    }
    esp32sim::I2CResult on_read(uint8_t* buf, size_t len) override {
        // Return the bytes the master would see. Most chips auto-increment
        // a register pointer that was set by the last on_write.
    }
};
```

See [the dev guide on adding peripheral fakes](../../dev/how-to/add-a-new-peripheral-fake.md)
for the full pattern, datasheet-conformance tests, and how to contribute the
fake back to the project.

## What this does and doesn't catch

The sim models the **register-level protocol**. It does not model:

- I2C clock-stretching, bus contention, or arbitration.
- Address-conflict detection across multiple devices at the same address.
- Electrical effects (pull-up sizing, capacitance, edge rates).

These remain real-hardware concerns. See [what this does and doesn't catch](../explanation/what-this-does-and-doesnt-catch.md).
