# How to add a new peripheral fake

You want to add a fake for an I2C or SPI chip not in our [peripherals
reference](../../user/reference/peripherals.md). Here's the pattern.

## Files to create

For a hypothetical `FakeFoo` over I2C at address 0x77:

```
include/peripherals/FakeFoo.h     # public header
src/peripherals/foo/FakeFoo.cpp    # implementation
test/test_peripherals_foo/test_foo.cpp  # datasheet-conformance tests
```

## Header skeleton

```cpp
// include/peripherals/FakeFoo.h
#pragma once
#include <esp32sim/i2c.h>
#include <cstdint>

namespace esp32sim::peripherals {

class FakeFoo : public esp32sim::I2CDevice {
public:
    static constexpr uint8_t DEFAULT_ADDR = 0x77;
    FakeFoo();

    // Test-side setters specific to your chip
    void setSomething(double value);

    // I2CDevice
    I2CResult on_write(const uint8_t* data, size_t len) override;
    I2CResult on_read(uint8_t* buf, size_t len) override;

    uint8_t reg(uint8_t addr) const { return regs_[addr]; }

private:
    uint8_t regs_[0x100] = {};   // sized to your chip's register space
    uint8_t reg_pointer_ = 0;
};

}  // namespace esp32sim::peripherals
```

## Implementation skeleton

```cpp
// src/peripherals/foo/FakeFoo.cpp
#include <peripherals/FakeFoo.h>

namespace esp32sim::peripherals {

FakeFoo::FakeFoo() {
    // Set datasheet defaults (chip ID, calibration coefficients, etc.)
    regs_[0xD0] = 0x42;  // example chip ID
}

void FakeFoo::setSomething(double v) {
    // Encode v into the appropriate raw register(s) using the chip's
    // documented formula.
}

I2CResult FakeFoo::on_write(const uint8_t* data, size_t len) {
    if (len == 0) return I2CResult::OK;
    reg_pointer_ = data[0];
    for (size_t i = 1; i < len; ++i) {
        regs_[reg_pointer_++] = data[i];
    }
    return I2CResult::OK;
}

I2CResult FakeFoo::on_read(uint8_t* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) buf[i] = regs_[reg_pointer_++];
    return I2CResult::OK;
}

}  // namespace esp32sim::peripherals
```

## Tests (datasheet-conformance)

The test suite for a fake should not test your code; it tests that the *fake*
behaves like the real chip would, per the datasheet. Five-ish tests:

1. Default register state matches power-on values.
2. Test API setter correctly populates the right registers.
3. I2C read pattern (master writes register pointer, then requests N bytes).
4. I2C write pattern (master writes register pointer + N data bytes).
5. Any chip-specific quirks (e.g., writing GPIOA on MCP23017 also updates OLATA).

Use Unity's `TEST_ASSERT_EQUAL_UINT8` etc. and the canonical test pattern
established by `test/test_peripherals_ds3231/test_ds3231.cpp`.

## Wire it into the build

Once your files are in `src/peripherals/foo/`, `include/peripherals/`, and
`test/test_peripherals_foo/`, PIO's standard library convention picks them up
automatically. No platformio.ini change needed.

## Document it

Add a row to `docs/user/reference/peripherals.md` with:
- Real chip name + manufacturer
- Header path
- Default I2C address
- Register coverage summary
- Test API
- Library compatibility (which Arduino libs work against it)
- Known gaps

## Submit

Run the full test suite: `pio test -e native`. If green, open a PR. Include
the datasheet section/page references in your commit message — useful for
future contributors validating the fake.
