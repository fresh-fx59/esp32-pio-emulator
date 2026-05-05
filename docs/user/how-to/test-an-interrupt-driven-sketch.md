# How to test an interrupt-driven sketch

Your sketch uses `attachInterrupt(pin, isr, RISING)` and you want to verify the ISR fires
correctly. Here's the pattern.

## The setup

In `esp32-pio-emulator`, ISRs fire **synchronously** when a pin's level changes in the
matching direction. There's no real preemption; the ISR runs in the middle of the
`Sim::gpio(N).setLevel(...)` call that caused the edge. This is a fidelity simplification
documented in [what this does and doesn't catch](../explanation/what-this-does-and-doesnt-catch.md).

## Example: counting button presses

Sketch:

```cpp
#include <Arduino.h>
volatile int press_count = 0;
void on_press() { press_count++; }

void setup() {
    pinMode(4, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(4), on_press, FALLING);
}

void loop() {}
```

Test:

```cpp
#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

extern int press_count;

void setUp(void)    { esp32sim::Sim::reset(); press_count = 0; }
void tearDown(void) {}

void test_press_increments_counter(void) {
    esp32sim::Sim::runSetup();          // attach the ISR
    TEST_ASSERT_EQUAL_INT(0, press_count);

    // Simulate a button press: HIGH (idle, pullup) → LOW (pressed)
    esp32sim::Sim::gpio(4).setLevel(LOW);
    TEST_ASSERT_EQUAL_INT(1, press_count);  // ISR fired synchronously

    // Release
    esp32sim::Sim::gpio(4).setLevel(HIGH);
    TEST_ASSERT_EQUAL_INT(1, press_count);  // RISING edge — ISR doesn't fire

    // Press again
    esp32sim::Sim::gpio(4).setLevel(LOW);
    TEST_ASSERT_EQUAL_INT(2, press_count);
}
```

## Tips

- **Reset your sketch's globals between tests.** `Sim::reset()` does not reset your sketch's
  global state (`press_count` in the example). Reset them in `setUp()` or your tests will
  leak state.
- **Use `Sim::gpio(N).pulse(level, ms)` for momentary inputs.** It sets a level, advances
  time, then restores the previous level. Useful for simulating a button press of a known
  duration.
- **Pin starts in pull state when `pinMode(INPUT_PULLUP)` is called.** So a pullup pin is
  HIGH at the start; you set it LOW to simulate "pressed."
- **`detachInterrupt` silences future calls** but doesn't fire any pending ones. The
  underlying listener remains attached internally; this is a known T2+ ergonomics issue.

## What this won't catch

If your sketch has a race between the ISR and the main loop — for example, the ISR sets a
flag and the main loop expects to see it on the next iteration — the synchronous-fire model
won't reproduce a real-hardware race where the ISR fires *during* a main-loop iteration.
Test those scenarios on real hardware.
