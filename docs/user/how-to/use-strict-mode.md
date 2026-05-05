# How to verify your sketch with strict mode (zero test authoring)

Strict mode runs your sketch in the simulator with **autonomous contract
enforcement**: the chip's well-known rules become the oracle. You don't
write assertions; the sim watches for violations and reports them.

## The pattern

Drop this generic test into your project's `test/` folder. It works against
any sketch:

```cpp
// test/test_verify/test_verify.cpp
#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void) {
    esp32sim::Sim::reset();
    esp32sim::Strict::instance().enable();
}
void tearDown(void) {}

void test_sketch_obeys_chip_contract(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop(20);  // run for 20 loop iterations of virtual time

    auto& strict = esp32sim::Strict::instance();
    if (strict.any()) {
        strict.print_report();
        TEST_FAIL_MESSAGE("sketch violates ESP32 chip contract; see report above");
    }
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_sketch_obeys_chip_contract);
    return UNITY_END();
}
```

Then run:

```bash
pio test -e native
```

Output looks like (when violations are found):

```
[strict-mode] 3 violation(s):
  [ESP_SIM_E001 @ t=0 us] digitalWrite on GPIO 4 with mode != OUTPUT — must call pinMode(4, OUTPUT) before digitalWrite
  [ESP_SIM_E021 @ t=12000 us] Wire.beginTransmission(0x99) — I2C addresses must fit in 7 bits (0x00..0x7F)
  [ESP_SIM_E040 @ t=15000 us] delayMicroseconds(20000) exceeds the 16383 hardware maximum on ESP32

FAIL: sketch violates ESP32 chip contract; see report above
```

Then look up each error code in
[`docs/user/reference/strict-mode.md`](../reference/strict-mode.md) for the
exact rule and remediation.

## What this catches

Authoritative chip-contract violations: pin restrictions, API call ordering
(begin-before-use), bus protocol violations, hardware limits, lifecycle
errors. ~20 rules in v1.1; growing in v1.2+.

## What this doesn't catch

**Intent-level bugs** — "publishes to the wrong topic," "watering algorithm
waters too long," "off-by-one in state machine." For those you still need
authored assertions (regular Unity tests). Strict mode is the *floor*, not
the ceiling.

## Selective enforcement

Test multiple scenarios with different strict-mode policies:

```cpp
void test_loop_doesnt_violate_chip_contract(void) {
    esp32sim::Sim::reset();
    esp32sim::Strict::instance().enable();
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop(50);
    TEST_ASSERT_EQUAL_size_t(0, esp32sim::Strict::instance().count());
}

void test_setup_publishes_correct_topic(void) {
    esp32sim::Sim::reset();
    // Strict mode OFF here; just want to assert on intent.
    esp32sim::Sim::runSetup();
    auto& publishes = esp32sim::Network::instance().mqtt_publishes();
    TEST_ASSERT_EQUAL_STRING("sensors/temp", publishes[0].topic.c_str());
}
```

## See also

- [Strict mode rule reference](../reference/strict-mode.md) — every error code with source citations.
- [What this catches and doesn't catch](../explanation/what-this-does-and-doesnt-catch.md).
