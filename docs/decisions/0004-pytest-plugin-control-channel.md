# ADR-0004 — pytest-embedded plugin control channel (alpha defers it)

| | |
|---|---|
| **Status** | Accepted |
| **Date** | 2026-05-05 |
| **Decision-maker** | fresh-fx59 (with Claude Opus 4.7), autonomous-mode decision |
| **Supersedes** | — |

## Context

T2 ships a `pytest-pio-emulator` plugin so scenario tests can drive an
`esp32-pio-emulator` sim from Python (the "second test surface" promised by
the master spec). The full vision includes Python-side time advancement,
peripheral attachment, and bidirectional control — but each of those
requires a control channel between the Python harness and the C++ sim
process. That's nontrivial scope.

T2 spec v0.2 (§"Spec drift expectations") flagged the plugin's exact
integration pattern as a likely drift point. Three options:

- **(a) stdout-only:** Python reads stdout via subprocess; cannot drive sim
  state. Smallest viable surface.
- **(b) Unix domain socket:** dedicated control channel; C++ sim spawns a
  listener thread; Python sends JSON-RPC commands.
- **(c) Stdin + structured prefix protocol on stdout:** repurpose stdin for
  control-in, stdout for both Serial output and control-out responses.
  Single-channel, no socket setup.

## Decision

**T2 alpha = (a) stdout-only.** No control channel. The plugin's surface is
just `dut.expect()`, `dut.expect_re()`, `dut.expect_exact()`, and `dut.buffer`.

T2.5 (post-T2 minor release) will revisit and likely choose **(b) Unix
domain socket** because:
- Cleaner separation between Serial (a sketch concern) and control (a sim
  concern).
- Easier to add commands incrementally without escaping issues.
- Standard pattern (matches QEMU's QMP, gdb's serial protocol, etc.).

## Consequences

**Good:**
- T2 alpha ships in days, not weeks. The plugin's value proposition (the
  *second test surface* exists, drives the same sketch) is delivered without
  the harder work.
- Stdout-only is the most common embedded-testing pattern; users coming from
  pytest-embedded against real hardware feel at home.

**Bad / accepted:**
- Tests that need Python-side time control or peripheral attach must wait
  for T2.5. For now, those test scenarios use the Unity surface (in-process,
  C++).
- The "same sketch, two surfaces" promise is delivered but slightly
  asymmetric — the Unity surface can do more than the pytest-embedded
  surface in alpha.

## Alternatives considered

1. **Ship with full control channel in T2** — rejected because T2 already
   has a lot of scope (3 peripheral fakes, 2 examples, plus the plugin
   itself). Better to land alpha and iterate.
2. **Skip the plugin entirely; pytest-embedded support starts at T2.5** —
   rejected because the master-spec architecture has *two test surfaces* as
   a foundational claim. Even an alpha is more honest than nothing.

## Self-critique

- *What could go wrong?* Users hit a wall at the alpha's limits and assume
  the framework can't do scenario tests properly.
- *Mitigation:* documentation is explicit about the alpha scope and what
  T2.5 will add. The README, the user how-to, and this ADR all spell it out.
- *Failure mode:* a user with a complex scenario test (e.g., needs to
  inject a clock advance mid-test) gives up. This is acceptable for v0.x;
  we'll prioritize T2.5 if real users hit it.

## Revisit at

- T2.5 entry — make the (b) Unix domain socket commitment formal at that
  ADR.
- Any time a user files an issue saying "I can't do X with the pytest
  plugin" where X is exactly something a control channel would enable.
