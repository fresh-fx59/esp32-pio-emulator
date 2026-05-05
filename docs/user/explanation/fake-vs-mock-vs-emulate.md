# Fake vs Mock vs Emulate

Three words for "test against a not-real thing." They mean different things
and shape your test design differently.

## Fake (what we do)

A *fake* is a working alternative implementation that behaves like the real
thing, just simpler. Our `FakeDS3231` actually maintains a register bank,
encodes time as BCD, and responds to I2C reads/writes the way a real DS3231
would. Your sketch can't tell the difference at the protocol level.

**Test style:** assert on outcomes. *"After my code reads the RTC, the
moisture log line has the right timestamp format."*

## Mock (what `ArduinoFake` does)

A *mock* is a recording instrument. It doesn't behave; it remembers. Tests
assert on *which calls were made with which arguments*. `ArduinoFake` records
every call to `digitalWrite`, `Wire.write`, etc. — your code can verify
"I called `Wire.beginTransmission(0x68)` exactly once."

**Test style:** assert on intent. *"My code attempted to talk to the RTC at
address 0x68."*

## Emulate (what QEMU does)

An *emulator* runs your firmware's actual binary on a different
architecture. QEMU runs Tensilica/RISC-V instructions on x86 by interpreting
each opcode. Behaviors emerge from the real binary executing.

**Test style:** runs your unmodified compiled firmware. Slow, faithful.

## Trade-offs

| | Fake | Mock | Emulate |
|---|---|---|---|
| Speed | Fast (millis) | Fast | Slow (seconds) |
| Setup | Construct + attach | Per-test mock setup | Image build, full toolchain |
| Catches | Logic + protocol bugs | Call-pattern bugs | All bugs except hardware |
| Misses | Register-level edge cases | Behavioral semantics | Speed + ergonomics |
| Best at | Fast TDD | Specifying interactions | Final integration test |

## Our position

We're a fake-based simulator. We chose this because the test-candidate
projects' workflows benefit most from "fast TDD against a sketch that
behaves" — not "did my code call X" verification. If you genuinely need
call-pattern verification, you can add `ArduinoFake` to your own
`lib_deps` (see [our framework vs ArduinoFake](our-framework-vs-arduinofake.md)).
