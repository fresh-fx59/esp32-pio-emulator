# ADR-0002 — Coexistence with ArduinoFake

| | |
|---|---|
| **Status** | Superseded by [ADR-0003](0003-supersede-arduinofake-coexistence.md) (same day) |
| **Date** | 2026-05-05 |
| **Decision-maker** | fresh-fx59 (with Claude Opus 4.7), autonomous-mode decision |
| **Supersedes** | — |
| **Superseded by** | [ADR-0003](0003-supersede-arduinofake-coexistence.md) — operator feedback that this ADR over-rotated toward backward-compat at the cost of design clarity. See ADR-0003 for the corrected position. |

## Context

[`fabiobatsilva/ArduinoFake`](https://github.com/FabioBatSilva/ArduinoFake) is a FakeIt-based mocking library for Arduino, used in the user's existing ESP32 projects (`iot-yc-water-the-flowers-mini`, `iot-yc-water-the-flowers`). It provides **call-recording mocks** — assertions like *"`digitalWrite` was called with pin=2 and value=HIGH"*. It's well-maintained, MIT-licensed, on the PlatformIO library registry as #1689, and works alongside Unity in PlatformIO's native test env.

`esp32-pio-emulator` provides a different abstraction: **behavioral fakes**. Where ArduinoFake records that `digitalWrite` was called, we *simulate the pin actually being driven HIGH*; subsequent reads return HIGH; downstream peripherals see the change. Where ArduinoFake records `Wire.write(0xD0, 0x58)`, we route the I2C transaction to a fake BMP280 that returns its chip ID.

These two tools answer different test questions:

| Question | Tool |
|---|---|
| "Was `digitalWrite(2, HIGH)` called exactly twice?" | ArduinoFake |
| "After running `loop()` for 500 ms of virtual time, what level is GPIO 2?" | esp32-pio-emulator |
| "Did my code call `Wire.beginTransmission(0x76)` before `Wire.write`?" | ArduinoFake |
| "When my code reads from BMP280 register 0xFA, what value does the parser see?" | esp32-pio-emulator |

So the question is: do we **replace** ArduinoFake (force users to choose), or do we **coexist** (let users use both, picking whichever fits each test)?

## Decision

**Coexist.** From T0 onward, our `[env:native]` PlatformIO config includes `fabiobatsilva/ArduinoFake @ ^0.3.1` in `lib_deps` alongside our own headers. Users can:

- Use `ESP32Sim::*` (our API) for behavioral assertions.
- Use ArduinoFake's `When(Method(ArduinoFake(), digitalWrite)).AlwaysReturn();` for call-recording assertions.
- Use both in the same Unity test.

Our public API and ArduinoFake's API don't collide — we install our headers under different paths and only intercept the symbols we behaviorally simulate; ArduinoFake's call recording wraps those same symbols at the *call-site* level inside individual tests.

## Consequences

**Good:**
- Users with existing ArduinoFake-based test suites can adopt us **incrementally** — add a few `ESP32Sim::*` assertions to existing tests without rewriting them. Zero migration cost for the user's test-candidate repos.
- We don't have to reimplement call-recording functionality (FakeIt already does it well).
- ArduinoFake and our framework are *literally* complementary — one captures *intent* (what your code asked the hardware to do), one captures *effect* (what would have happened in response).

**Bad / accepted:**
- Two-tool story to explain. Documentation must clearly say *when to use which*. Mitigated by `docs/user/explanation/our-framework-vs-arduinofake.md` (T1 deliverable).
- Slight binary-size hit in the test binary (negligible — these are test binaries, not embedded ones).
- Risk: ArduinoFake stops being maintained, our users hit unfixable issues. Last release is mid-2023, so this risk is real. Mitigation: if ArduinoFake stalls, we can absorb its functionality (FakeIt is the actual mocking engine — we'd add a thin wrapper, not reimplement).

## Alternatives considered

1. **Replace ArduinoFake with our own mocking facility** — rejected. Users have working setups; forced migration is hostile. We'd reinvent FakeIt (poorly). And call-recording vs behavioral simulation are *different abstractions*; mashing them into one tool is worse than two clean tools.
2. **Ignore ArduinoFake's existence** — rejected. The user already uses it. If we don't say anything, users will wonder why their existing test infrastructure is sidelined.
3. **Provide a compatibility shim** that lets ArduinoFake assertions work against our sim's recorded events — possible but speculative. Wait for a real user need.

## Self-critique (decide-under-uncertainty)

- *What could go wrong?* A user includes both, gets symbol conflicts at link time. Two mocking layers fight over `digitalWrite`'s definition.
- *Failure mode?* Compilation error or, worse, silent test corruption (our behavioral fake gets bypassed because ArduinoFake also wraps the symbol).
- *Mitigation?* Test this in T0. Acceptance criterion: a Unity test that uses both `ESP32Sim::gpio(2).level()` (ours) and `Verify(Method(ArduinoFake(), digitalWrite)).Once()` (ArduinoFake) on the same test must compile, link, and produce coherent assertions. If it doesn't, the coexistence promise is hollow and we need to revisit.
- *Would another option be safer?* "Replace" would have us controlling the whole stack but at user-migration cost. "Ignore" leaves it ambiguous. "Coexist with verified compatibility" is the right balance — but it's load-bearing and must be tested in T0.

## Revisit at

- T0 acceptance — verify the coexistence claim with an actual passing test.
- Any time ArduinoFake is unmaintained for >12 months and starts breaking against newer toolchains.
