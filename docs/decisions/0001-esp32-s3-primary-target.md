# ADR-0001 — ESP32-S3 as primary target

| | |
|---|---|
| **Status** | Accepted |
| **Date** | 2026-05-05 |
| **Decision-maker** | fresh-fx59 (with Claude Opus 4.7), autonomous-mode decision |
| **Supersedes** | — |

## Context

The arduino-esp32 framework supports a family of ESP32 variants — classic ESP32, S2, S3, C3, C5, C6, H2, P4 — with mostly-but-not-entirely uniform peripheral APIs. As a behavioral simulator that reproduces this API surface, we have to choose which variant our fakes target *primarily* (i.e., what register layouts, peripheral counts, and chip features we mirror) when variants disagree.

The two real test-candidate projects from the user's portfolio — `iot-yc-water-the-flowers-mini` and `iot-yc-water-the-flowers` — both target `esp32-s3-devkitc-1` exclusively. This is the practical context that motivated the decision.

## Decision

**ESP32-S3 is the primary target.** When variant-specific behavior matters (peripheral counts, pin maps, USB-OTG presence, RGB-LED on GPIO48, RMT channel counts, ADC pin counts), our fakes mirror ESP32-S3.

Other variants — classic ESP32, S2, C3, C5, C6, H2, P4 — are **best-effort**: most peripheral *APIs* are identical across variants in arduino-esp32, so most user code targeting any variant will also work in our sim. Variant-specific differences are documented as gaps; sketches that depend on them must test on real hardware for those parts.

## Consequences

**Good:**
- Concrete, testable target. We're not trying to be all things to all chips.
- Matches the user's own work — the test-candidate projects are real acceptance criteria.
- Reduces design churn: when arduino-esp32 abstracts over variants we follow it; when it doesn't, we pick S3.

**Bad / accepted:**
- Users on classic ESP32 / C3 / etc. may hit silent fidelity gaps where peripheral counts or pin maps differ. We'll discover and document these as the test suite grows.
- We don't get to advertise "all ESP32 variants supported" until we've genuinely verified that.

## Alternatives considered

1. **Classic ESP32 primary** — historically the most common variant in tutorials and community code. Rejected because the user's own work is on S3 and the framework should serve real users first; and because S3 is rapidly becoming the new default for new ESP32 projects.
2. **No primary; abstract over all variants from day 1** — rejected because variants disagree on real things (peripheral counts, pin maps), so "no primary" really means "S3 by accident with bugs everywhere else." Better to be explicit.
3. **Multi-variant matrix from day 1** — rejected for v1 scope. Too much work for too little payoff before we even ship T1. Revisit at T2 entry when peripheral fakes start to differ across variants.

## Self-critique (decide-under-uncertainty)

- *What could go wrong?* A user on classic ESP32 hits a subtle bug where, say, GPIO 35 is input-only on classic ESP32 but bidirectional on S3, and our sim treats it as bidirectional. They report the bug; it's a fidelity gap, not a feature.
- *Failure mode?* User wastes time chasing a sim-vs-hardware mismatch.
- *Mitigation?* Document the variant matrix prominently in `docs/user/explanation/variant-fidelity.md`. Add CI tests for any variant-specific behavior we model. When we hit gaps, file them as known-issues.
- *Would another option be safer?* "Classic ESP32 primary" would just shift the same problem to S3 users. "Multi-variant from day 1" would delay value indefinitely. Sticking with the decision.

## Revisit at

- T2 entry — when peripheral fakes start to differ per variant.
- Whenever a non-S3 user reports a fidelity gap that justifies promoting their variant to first-class.
