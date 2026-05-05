# CLAUDE.md — task-handoff context

This file is the **first thing** any agent (Claude Code, Codex, Gemini, human contributor)
should read when picking up work on this repo. It complements `AGENTS.md` (durable
preferences) with the *current* state of work.

## Project mission

Build `esp32-pio-emulator`: a behavioral simulator for ESP32 firmware that runs natively on
a developer's machine. The user's unmodified Arduino sketch compiles against host-side
fakes; Unity tests (and from T2, pytest-embedded scenario tests) assert on observable
behavior — pin levels, serial output, peripheral interactions, virtual time. See
[`docs/superpowers/specs/2026-05-05-esp32-pio-emulator-master-design.md`](docs/superpowers/specs/2026-05-05-esp32-pio-emulator-master-design.md)
for the full design.

## Current state

- **Active tier:** T4 (full chip — FS, NVS, deep-sleep, BLE, RTOS) — starting.
- **Last shipped tier:** T3 networked, sign-off 2026-05-05. CHANGELOG [0.4.0]. Tag v0.4.0.
- **Last verified:** ~136 tests green (T0-T3 inclusive).

See [`docs/superpowers/specs/2026-05-05-tier-3-networked-design.md`](docs/superpowers/specs/2026-05-05-tier-3-networked-design.md)
for the current T3 spec (still v0.1 sketch; refresh to v0.2 before writing the T3 plan, per spec-drift policy).

## What lives where

| Path | Purpose |
|---|---|
| `docs/superpowers/specs/` | Design specs (master + per-tier) |
| `docs/superpowers/plans/` | Implementation plans |
| `docs/decisions/` | ADRs (immutable architecture decisions) |
| `docs/user/` | User docs (Diátaxis: tutorials/how-to/reference/explanation) |
| `docs/dev/` | Contributor docs (Diátaxis) |
| `core/` | Behavioral sim engine (T1+) — virtual clock, event log, bus simulators |
| `platforms/arduino-esp32/` | Arduino HAL fakes (T1+) |
| `peripherals/` | Stateful peripheral fakes (T2+) — BMP280, MCP23017, etc. |
| `harness/unity/` | C++ Unity assertion API (T1+) |
| `harness/pytest_pio_emulator/` | Python pytest-embedded plugin (T2+) |
| `examples/` | End-to-end reference sketches per tier |
| `test/` | Framework's own self-tests |

## What NOT to touch yet

- Anything under `core/`, `platforms/`, `peripherals/`, `harness/`, `examples/` — those
  directories don't exist yet; T0 is scaffolding only.
- The other tier specs (T1–T4) — they're sketches that will be rewritten at their tier's
  entry per the spec-drift policy.

## Workflow

- `AGENTS.md` defines durable preferences (subagent-driven plan execution, TDD default,
  small commits, per-step verify-and-document, etc.).
- Each tier follows: refresh spec → write plan → execute plan (subagent-driven) → verify →
  commit → push → next tier.
- Operator authorized sustained autonomous mode for this project — do not prompt between
  phases. Surface only on real blockers or destructive actions.

## Quick verification

The smoke test for "is this repo healthy at the current tier" is:

```bash
pio test -e native           # green = T0+ skeleton works
```

(After T0 ships. Before then, the equivalent is checking that all 14 T0 plan tasks are
committed.)
