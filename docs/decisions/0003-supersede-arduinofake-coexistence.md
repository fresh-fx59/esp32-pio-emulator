# ADR-0003 — Supersede coexistence with ArduinoFake

| | |
|---|---|
| **Status** | Accepted |
| **Date** | 2026-05-05 |
| **Decision-maker** | fresh-fx59 (with Claude Opus 4.7) |
| **Supersedes** | [ADR-0002](0002-arduinofake-coexistence.md) |

## Context

[ADR-0002](0002-arduinofake-coexistence.md) committed us to coexisting with [ArduinoFake](https://github.com/FabioBatSilva/ArduinoFake) — adding it to our `[env:native]` `lib_deps` from T0 onward and making the coexistence claim a *load-bearing T0 acceptance criterion* (a Unity test using both `ESP32Sim::*` and `ArduinoFake` had to compile, link, and produce coherent assertions).

After landing ADR-0002, the operator pushed back: *our framework's approach (compile-time interception + behavioral fakes) is cleaner and better than the existing ArduinoFake-based pattern*. Existing tests in their projects (`iot-yc-water-the-flowers`, `iot-yc-water-the-flowers-mini`) **may be rewritten** to use our cleaner approach. The framework should not bend its design around an inferior pattern just because incumbents use it.

In other words, ADR-0002 was over-rotated toward migration-friction-minimization at the cost of design clarity.

## Decision

**Supersede ADR-0002.** Concretely:

1. **We do not ship ArduinoFake as a dependency.** Our `[env:native]` `lib_deps` does not include `fabiobatsilva/ArduinoFake`. Users who want call-recording mocks alongside our behavioral fakes can add it to their own `lib_deps` themselves; we will not validate or test that combination.
2. **We do not own the coexistence claim.** No T0 acceptance criterion mentions ArduinoFake. No T1 acceptance criterion does either. We test our own correctness.
3. **We acknowledge ArduinoFake exists** in `docs/user/explanation/our-framework-vs-arduinofake.md` (T1 deliverable), framing it as *one alternative among many* (others: GoogleMock, Hippomocks, Trompeloeil) — not as a partner. The doc explains the conceptual difference (call-recording vs behavioral) so users can make an informed choice if they want both.
4. **The user's existing test repos can be rewritten** using our framework as their adoption story. We don't ship migration tooling; the rewrite is straightforward and producing a clean target codebase is more valuable than tool support.

## Consequences

**Good:**
- Simpler `lib_deps`, simpler `platformio.ini`, simpler T0 acceptance.
- Our framework's value proposition isn't muddied by "and also works with this other thing."
- We don't carry the maintenance burden of "make sure we still play nice with ArduinoFake's latest release."
- Documentation is sharper: behavioral simulation as the recommended path, not behavioral-as-one-option-among-two.

**Bad / accepted:**
- A user with a large existing ArduinoFake-based test suite and zero appetite for rewriting may find friction adopting us. They can still add ArduinoFake themselves; we just don't *promise* the combination works. This is acceptable — that user is not our primary target; users writing new tests are.
- We lose a marketing line ("works with your existing ArduinoFake tests"). Acceptable.

## Alternatives considered

1. **Keep ADR-0002 as-is** — rejected per operator feedback. Coexistence-as-requirement was solving a problem (existing tests) that the user explicitly said is not a problem (those tests can be rewritten).
2. **Drop ArduinoFake mention entirely from documentation** — rejected. ArduinoFake exists in the ecosystem; users will encounter it. Pointing at it (with the conceptual contrast) helps them choose; ignoring it wastes their lookup time.
3. **Provide an opt-in adapter that maps ArduinoFake assertions to our event log** — rejected as speculative. Wait for a real user need before building optional integration code.

## Self-critique (decide-under-uncertainty)

- *What could go wrong?* A user adopts both libraries against our advice, hits subtle conflicts, files an issue blaming us.
- *Failure mode?* We spend support cycles on a configuration we explicitly said we don't support.
- *Mitigation?* Documentation is unambiguous: "We do not test, support, or recommend using ArduinoFake alongside `esp32-pio-emulator`. If you do, you're on your own." Issues filed about mixed setups get closed with a pointer to the doc.
- *Would another option be safer?* "Keep coexistence" trades complexity *forever* for a small one-time adoption nicety. "Drop entirely" loses educational value. Sticking with this decision: explain the difference, don't promise interop.

## Revisit at

- If 5+ users independently file the same "I want to use both" request with concrete use cases, reconsider building an opt-in adapter.
