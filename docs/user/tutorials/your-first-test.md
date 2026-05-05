# Your first test in 5 minutes

This tutorial gets you from *zero* to a passing TDD test of an unmodified Arduino sketch
against `esp32-pio-emulator` — no hardware needed.

## Prerequisites

- PlatformIO (Core 6.x). If `pio --version` doesn't work, see the
  [contributor setup guide](../../dev/tutorials/setting-up-dev-environment.md) for installing
  in a project-local venv.
- A C++ compiler (Linux's `g++` or macOS's `clang++` is fine).

## 1. Make a project

```bash
mkdir my-blink-test && cd my-blink-test
mkdir -p src test/test_blink
```

## 2. Write the sketch — the regular way you'd write it

`src/main.cpp`:

```cpp
#include <Arduino.h>

void setup() {
    pinMode(2, OUTPUT);
}

void loop() {
    digitalWrite(2, HIGH);
    delay(500);
    digitalWrite(2, LOW);
    delay(500);
}
```

This is exactly what you'd write to flash to a real ESP32. We are not asking you to refactor
or extract anything.

## 3. Configure PlatformIO

`platformio.ini`:

```ini
[env:native]
platform = native
test_framework = unity
lib_compat_mode = off
test_build_src = true
lib_deps =
    https://github.com/fresh-fx59/esp32-pio-emulator
build_flags = -std=gnu++17
```

Three things to know about this config:
- `lib_compat_mode = off` — disables PIO's framework filter (the `[env:native]` env has no
  framework declared, but our library says `frameworks: arduino`).
- `test_build_src = true` — compiles your sketch into the test binary so tests can drive its
  `setup()` / `loop()`.
- `lib_deps` — pulls the framework from GitHub. First run is slow (clone); after that, fast.

## 4. Write the test

`test/test_blink/test_blink.cpp`:

```cpp
#include <Arduino.h>
#include <esp32sim_unity/esp32sim.h>
#include <unity.h>

void setUp(void)    { esp32sim::Sim::reset(); }
void tearDown(void) {}

void test_led_toggles(void) {
    esp32sim::Sim::runSetup();
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(LOW, esp32sim::Sim::gpio(2).level());

    esp32sim::Sim::advanceMs(500);
    esp32sim::Sim::runLoop();
    TEST_ASSERT_EQUAL_INT(HIGH, esp32sim::Sim::gpio(2).level());
}

int main(int /*argc*/, char** /*argv*/) {
    UNITY_BEGIN();
    RUN_TEST(test_led_toggles);
    return UNITY_END();
}
```

What this test says: "After `setup()` + one `loop()` iteration, pin 2 should be LOW. After
500 ms of virtual time and another `loop()`, pin 2 should be HIGH."

## 5. Run it

```bash
pio test -e native
```

You should see:

```
test/test_blink/test_blink.cpp:N: test_led_toggles    [PASSED]
1 test cases: 1 succeeded
```

That's it. You wrote an unmodified sketch, wrote a Unity test, and verified the sketch's
behavior on your laptop in seconds — no hardware, no flashing.

## What's next

- [Reference: the `esp32sim::Sim::*` API](../reference/sim-api.md)
- [How-to: testing an interrupt-driven sketch](../how-to/test-an-interrupt-driven-sketch.md)
- [Explanation: why virtual time?](../explanation/why-virtual-time.md)
