# `esp32-pio-emulator` vs `ArduinoFake` (and friends)

If you've written ESP32 native tests before, you've probably encountered
[`ArduinoFake`](https://github.com/FabioBatSilva/ArduinoFake), `EpoxyDuino`, or `EspMock`.
They solve a related problem in a different way. This page explains the difference so you
can make an informed choice.

## Two abstractions for "test ESP32 code on a host machine"

### Behavioral simulation (us)

Your sketch's `digitalWrite(2, HIGH)` actually drives a virtual pin to HIGH. `digitalRead(2)`
returns HIGH afterwards. Your code-under-test sees a *real-ish* hardware that responds. You
assert on what would have happened — *"after this `loop()` iteration, pin 2 was high for
exactly 500 ms"*.

### Call-recording mocks (`ArduinoFake`, FakeIt-based)

Your sketch's `digitalWrite(2, HIGH)` is recorded as a *call event*. The mock framework lets
you assert *"`digitalWrite` was called with pin=2, value=HIGH, exactly twice"*. The pin
doesn't actually go HIGH — the mock just remembers the call.

These are different abstractions. They answer different questions:

| Question | Tool |
|---|---|
| "What level is the pin after my code runs?" | esp32-pio-emulator |
| "Did my code call `digitalWrite` exactly twice?" | ArduinoFake |
| "When my BMP280 driver reads from register 0xFA, what value does it get?" | esp32-pio-emulator (T2+) |
| "Did my driver call `Wire.beginTransmission(0x76)` before `Wire.write`?" | ArduinoFake |

Both are valid. Most projects we've seen want the first column — *what does my code make the
chip do?* That's behavioral simulation.

## Compatibility / coexistence

We do not include `ArduinoFake` in our `lib_deps`. Per [ADR-0003](../../decisions/0003-supersede-arduinofake-coexistence.md),
we don't make `ArduinoFake` coexistence a load-bearing promise. If you want to use both, you
can add `ArduinoFake` to your own project's `lib_deps`, but we don't test or support that
combination. Realistically, you almost never need both — pick the abstraction that matches
the questions you're asking.

## What about EpoxyDuino?

[`EpoxyDuino`](https://github.com/bxparks/EpoxyDuino) compiles Arduino code on a Linux host
with stub Arduino headers. Its stubs are *deliberately empty* — the project's purpose is
"prove this compiles," not "exercise behavior." We borrow the include-layout pattern from
EpoxyDuino but our headers are *behavioral* (a `digitalWrite` actually changes pin state in
the sim).

## What about Wokwi / QEMU?

Wokwi is a hosted, closed-source circuit simulator. QEMU emulates the Tensilica/RISC-V CPU
and runs your real binary. Both are heavier than what we do. Use Wokwi for visual circuit
prototyping; use QEMU for cross-architecture binary testing; use us for fast TDD where you
care about behavior, not electrons.
