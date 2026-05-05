# Why virtual time?

`delay(500)` in your sketch does not sleep for half a second when run against
`esp32-pio-emulator`. It advances a *virtual clock* by 500 ms instantaneously. `millis()`
returns time in this virtual clock; so do `micros()` and any timer-based logic.

## Why not real-time?

Two reasons.

### 1. Test feedback would crawl

A typical ESP32 sketch has `delay()` calls measured in tens or hundreds of milliseconds
during normal operation. A debouncer might wait 50 ms; an LED blink loop waits 500 ms;
a sensor poll might wait 1000 ms; deep-sleep waits *hours*. Even a small test that exercises
five blink cycles would take 5 seconds of real time. Run a thousand such tests in CI and you
need a coffee break for every PR.

With virtual time, the same test runs in milliseconds. The whole point of TDD is that the
*feedback loop* is fast enough to keep you in flow. Real-time `delay()` breaks that.

### 2. Tests become flaky on shared infrastructure

A `delay(500)` followed by a `digitalRead` assumes 500 ms have actually passed. On a
heavily-loaded CI runner, the OS might preempt your process and 600 ms passes between the
delay and the read. If the test asserts on what should have happened *exactly* at 500 ms,
it fails — but only sometimes, and only on CI. That's flake-by-design.

Virtual time is deterministic. The clock advances by exactly what you asked for. A test
that passes once passes always.

## The cost

You give up one thing: the ability to discover concurrency / preemption bugs in your sketch.
If your sketch has a race between a periodic `loop()` action and an ISR-driven update,
real hardware will hit it in production but the simulator will not — because there's no
real preemption.

That's a fidelity gap, not a flaw. The simulator catches most firmware bugs and runs in
milliseconds. Real-hardware QA catches the residual concurrency bugs. Both are part of a
healthy test pipeline.

## See also

- [What this does and doesn't catch](what-this-does-and-doesnt-catch.md) — the honest list
  of fidelity limits.
- [`Sim::advanceMs` reference](../reference/sim-api.md#time) — how tests drive the clock.
