# Tier 0 Skeleton — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Stand up the empty `esp32-pio-emulator` repository with everything Tier 1 needs to begin — but no simulator code yet. After T0, a contributor can `git clone`, `pio test -e native`, and see a single trivial Unity test pass on Linux, macOS, and in CI.

**Architecture:** PlatformIO library project with a single `[env:native]` test environment using Unity. Repository serves as a library that consumers depend on via `lib_deps` — we don't build firmware in this repo. Diátaxis-organized docs from day 1. Pre-commit hooks enforce style. GitHub Actions runs `pio test -e native` on Ubuntu + macOS.

**Tech Stack:** PlatformIO, Unity (PIO default test framework), C++ (gnu++17), Python (placeholder for T2), GitHub Actions, pre-commit framework, clang-format.

**Reference docs (read before starting):**
- [Master design spec](../specs/2026-05-05-esp32-pio-emulator-master-design.md)
- [Tier 0 design spec](../specs/2026-05-05-tier-0-skeleton-design.md)
- [ADR-0001 — ESP32-S3 primary target](../../decisions/0001-esp32-s3-primary-target.md)
- [ADR-0003 — Supersede ArduinoFake coexistence](../../decisions/0003-supersede-arduinofake-coexistence.md)

**Working directory:** `/home/claude-developer/esp32-pio-emulator`

**Convention reminders (from AGENTS.md, applies to every task):**
- One concept per commit. Never `--no-verify`. Never amend a published commit.
- Every step ends with: (1) verification command run + green, (2) docs the change touched updated in same commit series, (3) commit + push.
- `git add -A` is acceptable in this repo at this stage; we have a strict `.gitignore`.
- Tasks 1–14 below are the discrete plan steps. Each task = one commit (or a tightly coupled mini-series).

---

## Task 1: LICENSE + initial README + CHANGELOG

**Files:**
- Create: `LICENSE`
- Create: `README.md`
- Create: `CHANGELOG.md`

- [ ] **Step 1: Create `LICENSE` (MIT)**

```
MIT License

Copyright (c) 2026 fresh-fx59 and esp32-pio-emulator contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

- [ ] **Step 2: Create `README.md`**

```markdown
# esp32-pio-emulator

> Status: **Tier 0 in progress** · License: MIT · ESP32-S3 primary target · gnu++17

A behavioral simulator for ESP32 firmware that runs natively on a developer's machine.
Compile your unmodified Arduino sketch against host-side fakes of the ESP32 hardware
abstraction layer; run Unity tests; assert on what your code makes the chip do — without
flashing a board.

**Why:** the ESP32 testing landscape has fast-but-shallow (manual mocks), slow-but-broad
(real hardware, Wokwi, QEMU), or fast-but-closed (Wokwi). This project targets the
**fast + broad + open-source + locally-runnable** quadrant.

## Status

This repository is in active early development. See [`docs/superpowers/specs/`](docs/superpowers/specs/)
for design specs and [`docs/decisions/`](docs/decisions/) for ADRs.

| Tier | Capability | Status |
|---|---|---|
| T0 | Skeleton (this tier) | 🚧 in progress |
| T1 | GPIO TDD | ⏸ blocked on T0 |
| T2 | Sensor TDD + pytest-embedded plugin | ⏸ |
| T3 | Networked ESP32 (WiFi, HTTP, MQTT) | ⏸ |
| T4 | Full chip (filesystem, NVS, deep-sleep, BLE, RTOS) | ⏸ |

## Getting started

(After T1 ships.) Add to your PlatformIO project's `[env:native]`:

```ini
[env:native]
platform = native
test_framework = unity
lib_deps =
    https://github.com/fresh-fx59/esp32-pio-emulator
build_flags = -std=gnu++17
```

## Documentation

- [Specs](docs/superpowers/specs/) — design rationale and tier roadmap
- [Decisions](docs/decisions/) — architecture decision records
- [User docs](docs/user/) — how to write tests against the simulator
- [Developer docs](docs/dev/) — how to contribute to the framework

## License

MIT — see [LICENSE](LICENSE).
```

- [ ] **Step 3: Create `CHANGELOG.md`**

```markdown
# Changelog

All notable changes to this project will be documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial repository bootstrap, license, README, changelog.

## [0.0.0] — 2026-05-05

### Added
- Project conceived. Master design spec and tier specs T0–T4 written and committed.
- ADR-0001 (ESP32-S3 primary target) and ADR-0003 (no ArduinoFake coexistence) accepted.
```

- [ ] **Step 4: Verify the three files exist and parse**

```bash
test -f LICENSE && test -f README.md && test -f CHANGELOG.md && \
  grep -q "MIT License" LICENSE && \
  grep -q "esp32-pio-emulator" README.md && \
  grep -q "Keep a Changelog" CHANGELOG.md && \
  echo "OK"
```

Expected: prints `OK`.

- [ ] **Step 5: Commit**

```bash
git add LICENSE README.md CHANGELOG.md
git commit -m "$(cat <<'COMMIT'
chore: license, readme, changelog

T0 step 1: project metadata files. README points at design specs and
ADRs; CHANGELOG seeded with 0.0.0 reflecting the spec-only state.
COMMIT
)"
```

---

## Task 2: AGENTS.md — operator preferences

**Files:**
- Create: `AGENTS.md`

- [ ] **Step 1: Create `AGENTS.md`** — adapted from the user's MPA template (`/home/claude-developer/esp32-pio-emulator/.source-agents-md.reference`). Keep §Execution defaults, §Documentation currency philosophy, §Safety. Replace project-specific tables (β scope, Telegram, host inventory, cliproxyapi quirks) with esp32-pio-emulator equivalents.

```markdown
# AGENTS.md — durable operator preferences for esp32-pio-emulator

This file captures operator-provided defaults that Claude Code (and other agents) should
apply without asking again. It complements `CLAUDE.md` (task-handoff context) with stable,
project-wide rules.

## Execution defaults

- **Plan execution mode: subagent-driven.** Dispatch one fresh subagent per plan task, with
  a review checkpoint between tasks. Do not ask the user to choose execution mode — proceed
  with subagents unless the user explicitly overrides.
- **TDD is the default** for original code we write (the C++ core, platform adapter, harness
  code, and — from T2 — the Python pytest-embedded plugin). Configuration files
  (`platformio.ini`, `library.json`, CI YAML) are validated by their build/run output, not
  by TDD.
- **Small, reviewable commits.** One concept per commit. Never `--no-verify`, never
  `--force-push` to shared branches, never amend a published commit.
- **Commit before every stage / phase boundary.** Flush all pending work to `main` before
  starting the next plan phase / tier so the operator can inspect diffs between phases. No
  uncommitted work crosses a phase transition.
- **Don't prompt between phases.** Self-inspect, report concisely, and proceed. Surface only
  on real blockers or destructive actions.
- **Per-step verify-and-document.** After implementing each plan task: (1) verify the change
  behaves as intended (`pio test -e native`, build success, hook output, etc.); (2) update
  every doc the change affects in the same commit series (the relevant tutorial, how-to,
  reference, ADR, CHANGELOG); (3) commit + push. Don't start the next task until the current
  is verified-and-pushed.
- **Spec drift policy.** If the spec assumes something that reality contradicts, commit a
  spec update *first* (separate commit), then adapt the implementation. Specs in
  `docs/superpowers/specs/` are versioned via their changelog table; bump from v0.1 → v0.2
  when changing.
- **Decide-under-uncertainty: don't stall, self-critique once, pick.** When a decision is
  ambiguous and we're in autonomous mode: re-read AGENTS.md / the active spec; apply best
  practice for the domain (embedded, C++, Python, security); pick the most defensible
  option; *self-critique once* (one short paragraph: what could go wrong, what's the
  failure mode, would another option be safer); commit to the choice and document it in
  the commit message body or — if architecturally significant — as an ADR. Escalate to the
  operator only on genuinely irreversible decisions (license selection, public-release
  blockers, data deletion).
- **Subagent-first dispatch.** When a plan task is well-scoped (clear spec, isolated files,
  clear acceptance), dispatch a fresh subagent rather than working inline. Reserve inline
  work for coordination, design decisions, code review, and stitching subagent outputs.
- **Critique-once on every questionable step.** Operator explicitly reinforced this: not
  just headline architectural decisions, but ordinary implementation choices (mocking
  library, CI image, naming convention) get the one-pass critique when the right move
  isn't obvious. Do not turn this into a debate loop.

## Documentation currency

Docs are organized under `docs/` following [Diátaxis](https://diataxis.fr/) (tutorials /
how-to / reference / explanation) split by audience (`user/` for test authors, `dev/` for
framework contributors). See [`docs/README.md`](docs/README.md) for the map.

**Keep docs in sync with code — in the same commit.** When any of the following change,
update the corresponding doc as part of the same change. Stale docs are worse than missing
docs; don't let them drift.

| Change | Touch these docs |
|---|---|
| New public API in `core/include/esp32sim/` | `docs/user/reference/sim-api.md` (or peripheral-specific reference) |
| New supported Arduino API in `platforms/arduino-esp32/` | `docs/user/reference/supported-arduino-apis.md` |
| New peripheral fake under `peripherals/` | `docs/user/reference/peripherals.md`, `docs/user/how-to/fake-an-i2c-sensor.md` (T2+) |
| Test-authoring pattern changes | `docs/user/tutorials/your-first-test.md`, relevant how-tos |
| Pytest plugin behavior changes (T2+) | `docs/user/how-to/use-pytest-embedded.md`, `docs/dev/reference/control-protocol.md` |
| ESP32 variant fidelity gap discovered | `docs/user/explanation/variant-fidelity.md` |
| New tier shipped | master design spec § "Tier roadmap" status, `README.md` status table, `CHANGELOG.md` |
| Significant architectural choice made or reversed | New ADR under `docs/decisions/NNNN-<slug>.md` *before* the implementing commit lands. ADRs are immutable — supersede with new ADR, never edit. |
| Plan / spec changes | bump version + changelog table inside the doc |
| New top-level folder appears or audience split shifts | `docs/README.md` |

**Process reality check.** If you run a procedure (test invocation, install command, CI
config) and the doc doesn't match what you actually did, the doc is wrong — fix it in the
same PR.

**Audience discipline.** A "how to write a test" doc goes in `docs/user/`. A "how to add a
peripheral fake" doc goes in `docs/dev/`. If the same fact lives in two places, one of the
copies must be a link, not a duplicate — duplicated facts will drift.

## Project scope answers (β-equivalent — fill as the project grows)

| Question | Answer |
|---|---|
| Primary target ESP32 variant | **ESP32-S3** ([ADR-0001](docs/decisions/0001-esp32-s3-primary-target.md)) |
| Primary framework | Arduino-on-ESP32 (`framework = arduino`, `platform = espressif32`); ESP-IDF adapter is post-v1 |
| C++ standard | `gnu++17` |
| Test framework | Unity (PlatformIO default) for in-process tests; pytest-embedded service plugin for out-of-process scenario tests (T2+) |
| Python tooling | `ruff` for lint+format. Lands in T2 with the pytest-embedded plugin. |
| ArduinoFake position | Not shipped as a `lib_deps`. See [ADR-0003](docs/decisions/0003-supersede-arduinofake-coexistence.md). |
| License | MIT (see `LICENSE`) |
| Supported host OS | Ubuntu 22.04+, macOS 13+. Windows via WSL2 documented as untested until verified. |

## Reference projects (test candidates)

These real ESP32 projects from the operator's portfolio drive per-tier acceptance:

| Repo | Role |
|---|---|
| [`fresh-fx59/iot-yc-water-the-flowers-mini`](https://github.com/fresh-fx59/iot-yc-water-the-flowers-mini) | Single-zone watering; ESP32-S3, gnu++17. Smallest realistic target. |
| [`fresh-fx59/iot-yc-water-the-flowers`](https://github.com/fresh-fx59/iot-yc-water-the-flowers) | Multi-tray watering, self-learning; ESP32-S3, gnu++11 (will migrate). |

Each tier's acceptance includes "this subset of code from these repos runs in the sim." See
[master spec §11.5](docs/superpowers/specs/2026-05-05-esp32-pio-emulator-master-design.md).

## Safety

- **Never commit secrets.** `.env`, `*.pem`, `*.key`, `secrets/` are in `.gitignore`. There
  are currently no project secrets — this rule guards against accidental ones.
- **Never modify another project's repository as a side-effect of work in this one.** If
  changes to test-candidate repos (`iot-yc-water-the-flowers*`) become necessary as a
  consequence of framework decisions, raise a separate change in those repos — do not
  mix-cross-repo edits in a single commit.
- **Never publish a release until** every "deferred" decision in the master spec's register
  is resolved (license confirmation, registry namespace, etc.).
```

- [ ] **Step 2: Verify file exists, links resolve, no MPA-specific facts leak**

```bash
test -f AGENTS.md && \
  grep -c "MPA\|cliproxyapi\|OVH\|Contabo\|Telegram\|45\.151\.30\.146\|31\.220\.78\.216" AGENTS.md
```

Expected: prints `0` (no MPA-specific leakage).

- [ ] **Step 3: Commit**

```bash
git add AGENTS.md
git commit -m "$(cat <<'COMMIT'
chore: AGENTS.md — durable operator preferences

T0 step 2. Adapted from MPA AGENTS.md template:
- Kept Execution defaults, Documentation currency philosophy, Safety,
  Decide-under-uncertainty, Critique-once doctrines.
- Replaced MPA-specific tables (β scope, hosts, cliproxyapi quirks,
  Telegram test account, observability) with esp32-pio-emulator
  equivalents (project scope answers, reference projects).
- Added explicit pointers to ADR-0001 and ADR-0003 for
  variant-target and ArduinoFake position decisions.
- Documentation-currency table customized for the framework's actual
  surface (peripheral fakes, sim API, pytest plugin, etc.).

This file is durable — agents apply these preferences without asking.
COMMIT
)"
```

---

## Task 3: CLAUDE.md — task-handoff context

**Files:**
- Create: `CLAUDE.md`

- [ ] **Step 1: Create `CLAUDE.md`**

```markdown
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

- **Active tier:** T0 (skeleton). See [`docs/superpowers/specs/2026-05-05-tier-0-skeleton-design.md`](docs/superpowers/specs/2026-05-05-tier-0-skeleton-design.md)
  and [`docs/superpowers/plans/2026-05-05-tier-0-skeleton.md`](docs/superpowers/plans/2026-05-05-tier-0-skeleton.md).
- **Last committed work:** spec + ADR drop, no implementation code yet.
- **Next tier (T1):** GPIO TDD. Spec is written but will be refreshed at T1 entry.

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
```

- [ ] **Step 2: Commit**

```bash
git add CLAUDE.md
git commit -m "$(cat <<'COMMIT'
chore: CLAUDE.md — task-handoff context

T0 step 3. Companion to AGENTS.md: AGENTS.md is durable, CLAUDE.md is
current-state. Lists active tier, next tier, file map, what-not-to-touch,
and quick-health-check command. Updated at every tier boundary.
COMMIT
)"
```

---

## Task 4: Code style configs (.clang-format, .editorconfig, pyproject.toml)

**Files:**
- Create: `.clang-format`
- Create: `.editorconfig`
- Create: `pyproject.toml`

- [ ] **Step 1: Create `.clang-format`** — LLVM-derived, 100-col, indent=4 (matches user's existing C++ style):

```yaml
---
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
PointerAlignment: Left
AccessModifierOffset: -4
NamespaceIndentation: None
AllowShortFunctionsOnASingleLine: InlineOnly
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
BreakBeforeBraces: Attach
SortIncludes: CaseInsensitive
IncludeBlocks: Preserve
SpaceBeforeParens: ControlStatements
Cpp11BracedListStyle: true
Standard: c++17
```

- [ ] **Step 2: Create `.editorconfig`**:

```ini
root = true

[*]
indent_style = space
indent_size = 4
end_of_line = lf
charset = utf-8
trim_trailing_whitespace = true
insert_final_newline = true

[*.{md,yml,yaml,json}]
indent_size = 2

[Makefile]
indent_style = tab
```

- [ ] **Step 3: Create `pyproject.toml` (stub for T2)**:

```toml
[project]
name = "esp32-pio-emulator-tools"
description = "Python tooling for esp32-pio-emulator (pytest-embedded plugin lands in T2)."
version = "0.0.0"
requires-python = ">=3.10"

[tool.ruff]
line-length = 100
target-version = "py310"

[tool.ruff.lint]
# Conservative defaults for now. T2 expands this when the pytest plugin lands.
select = ["E", "F", "W", "I", "B", "UP"]
ignore = []

[tool.ruff.format]
quote-style = "double"
indent-style = "space"
```

- [ ] **Step 4: Verify files exist**

```bash
test -f .clang-format && test -f .editorconfig && test -f pyproject.toml && echo "OK"
```

Expected: prints `OK`.

- [ ] **Step 5: Commit**

```bash
git add .clang-format .editorconfig pyproject.toml
git commit -m "$(cat <<'COMMIT'
chore: code style configs (clang-format, editorconfig, pyproject)

T0 step 4. LLVM-derived clang-format with 100 cols and 4-space indent
to match the user's existing C++ style. EditorConfig for cross-editor
consistency. pyproject.toml as a durable home for ruff config — Python
toolchain doesn't actually land until T2 (pytest-embedded plugin).
COMMIT
)"
```

---

## Task 5: Diátaxis docs scaffold

**Files:**
- Create: `docs/README.md` (the navigation map)
- Create: `docs/user/{tutorials,how-to,reference,explanation}/README.md` (4 files)
- Create: `docs/dev/{tutorials,how-to,reference,explanation}/README.md` (4 files)

- [ ] **Step 1: Create the eight Diátaxis subfolder READMEs**. Each is a stub that explains what *belongs* in that folder (per Diátaxis conventions) and lists what's currently there (initially empty bullet lists).

`docs/user/tutorials/README.md`:

```markdown
# User tutorials

Step-by-step lessons that take a complete beginner from zero to a working test against the
simulator. Tutorials are *learning-oriented* (per [Diátaxis](https://diataxis.fr/)): they're
designed to give the user confidence by getting something working, even if they don't
understand every detail yet. Tutorials commit to a happy path and don't list alternatives.

## Contents

(Empty until T1.)

- *Coming in T1:* `your-first-test.md` — write your first GPIO test in 5 minutes.
```

`docs/user/how-to/README.md`:

```markdown
# User how-to guides

Recipes for solving specific problems against the simulator. How-to guides assume the user
already knows the basics (i.e., has done the relevant tutorial) and just needs to know how
to accomplish a concrete task. They're *task-oriented* (per [Diátaxis](https://diataxis.fr/)).

## Contents

(Empty until T1.)

- *Coming in T1:* `test-an-interrupt-driven-sketch.md`
- *Coming in T2:* `fake-an-i2c-sensor.md`, `use-pytest-embedded.md`
- *Coming in T3:* `test-a-wifi-sketch.md`, `test-mqtt.md`
- *Coming in T4:* `test-deep-sleep.md`, `test-ble-provisioning.md`, `test-with-littlefs.md`
```

`docs/user/reference/README.md`:

```markdown
# User reference

Authoritative, minimal-prose listings of public API surface. Reference docs are
*information-oriented* (per [Diátaxis](https://diataxis.fr/)): the user is here because they
need to know exactly what a function does or which APIs are supported, not to learn or to
solve a problem.

## Contents

(Empty until T1.)

- *Coming in T1:* `sim-api.md` (the `ESP32Sim::*` API), `supported-arduino-apis.md`
- *Coming in T2:* `peripherals.md`, `sim-control-protocol.md`
- *Coming in T4:* `full-api-coverage.md`
```

`docs/user/explanation/README.md`:

```markdown
# User explanation

Background, design rationale, and discussion of the *why*. Explanation docs are
*understanding-oriented* (per [Diátaxis](https://diataxis.fr/)): the user is here to deepen
their mental model, not to do or learn a specific thing.

## Contents

(Empty until T1.)

- *Coming in T1:* `why-virtual-time.md`, `what-this-does-and-doesnt-catch.md`,
  `our-framework-vs-arduinofake.md`
- *Coming in T2:* `fake-vs-mock-vs-emulate.md`
- *Coming in T4:* `dual-core-and-rtos-fidelity.md`, `variant-fidelity.md`
```

`docs/dev/tutorials/README.md`:

```markdown
# Developer tutorials

Step-by-step lessons for first-time contributors to the framework. *Learning-oriented* (per
[Diátaxis](https://diataxis.fr/)).

## Contents

- `setting-up-dev-environment.md` — set up your dev environment (T0).
```

`docs/dev/how-to/README.md`:

```markdown
# Developer how-to guides

Recipes for common contributor tasks. *Task-oriented* (per [Diátaxis](https://diataxis.fr/)).

## Contents

(Empty until T1.)

- *Coming in T1:* `add-a-new-arduino-api.md`
- *Coming in T2:* `add-a-new-peripheral-fake.md`, `add-a-platform-adapter.md`
```

`docs/dev/reference/README.md`:

```markdown
# Developer reference

Internal architecture, ABIs, file layout, contracts. *Information-oriented* (per
[Diátaxis](https://diataxis.fr/)).

## Contents

(Empty until T2.)

- *Coming in T2:* `control-protocol.md` (Python harness ⇄ C++ binary protocol)
- *Coming in T2:* `repo-layout.md`
```

`docs/dev/explanation/README.md`:

```markdown
# Developer explanation

Design rationale for contributors. *Understanding-oriented* (per [Diátaxis](https://diataxis.fr/)).

## Contents

- `architecture-overview.md` — the framework's architecture in 10 minutes (T0).
- ADRs: see [`../../decisions/`](../../decisions/).
```

- [ ] **Step 2: Create `docs/README.md`** (the navigation map):

```markdown
# Documentation map

This project's docs follow [Diátaxis](https://diataxis.fr/), split by audience.

## Audiences

- **User** — you write ESP32 sketches and want to test them with this framework. Start at
  [`user/tutorials/`](user/tutorials/).
- **Dev** — you contribute to the framework itself. Start at
  [`dev/tutorials/setting-up-dev-environment.md`](dev/tutorials/setting-up-dev-environment.md).

## Categories (per Diátaxis)

- **Tutorials** — learning-oriented lessons.
- **How-to** — task-oriented recipes.
- **Reference** — information-oriented authoritative listings.
- **Explanation** — understanding-oriented background and rationale.

## Index

```text
docs/
├── user/
│   ├── tutorials/         (T1+) "your first GPIO test", etc.
│   ├── how-to/            (T1+) recipes for testing scenarios
│   ├── reference/         (T1+) sim API, supported Arduino APIs, peripheral list
│   └── explanation/       (T1+) why virtual time, framework vs ArduinoFake, etc.
├── dev/
│   ├── tutorials/         (T0)  setting-up-dev-environment
│   ├── how-to/            (T1+) add a new Arduino API, add a peripheral fake
│   ├── reference/         (T2+) control protocol, repo layout
│   └── explanation/       (T0+) architecture-overview
├── decisions/             ADRs (immutable; supersede with new ADR)
└── superpowers/
    ├── specs/             design specs (master + per-tier)
    └── plans/             implementation plans (one per tier)
```

## Audience discipline

Same fact in two docs: one of them must be a link, not a duplicate. Stale docs are worse
than missing docs (per [`AGENTS.md`](../AGENTS.md)).
```

- [ ] **Step 3: Verify all files**

```bash
ls docs/user/tutorials/README.md docs/user/how-to/README.md docs/user/reference/README.md docs/user/explanation/README.md docs/dev/tutorials/README.md docs/dev/how-to/README.md docs/dev/reference/README.md docs/dev/explanation/README.md docs/README.md && echo "OK"
```

Expected: lists all 9 files and prints `OK`.

- [ ] **Step 4: Commit**

```bash
git add docs/
git commit -m "$(cat <<'COMMIT'
docs: Diátaxis scaffold + navigation map

T0 step 5. Eight Diátaxis subfolders (user/dev × tutorials/how-to/
reference/explanation), each with a README explaining what belongs
there per Diátaxis conventions. docs/README.md is the navigation map
linking it all together. Each subfolder's README lists what's coming
in which tier so contributors can see the trajectory.
COMMIT
)"
```

---

## Task 6: Architecture overview doc (`docs/dev/explanation/architecture-overview.md`)

**Files:**
- Create: `docs/dev/explanation/architecture-overview.md`

- [ ] **Step 1: Create the file** — short prose version of master spec §4-§5, for contributors who don't want to read the whole spec:

```markdown
# Architecture overview

A 10-minute prose tour of how `esp32-pio-emulator` is built. For the canonical detailed
spec, read [`../../superpowers/specs/2026-05-05-esp32-pio-emulator-master-design.md`](../../superpowers/specs/2026-05-05-esp32-pio-emulator-master-design.md).

## What it is

A behavioral simulator: the user's ESP32 sketch is compiled *natively* on the host (Linux
or macOS) against drop-in replacements for the Arduino-ESP32 framework's headers. The
sketch runs as a regular host program. The simulator does **not** emulate the Tensilica or
RISC-V CPU; it intercepts the hardware abstraction layer and substitutes behaviorally-faithful
fakes.

This puts us in a different category from QEMU (CPU emulation) and Wokwi (hosted, circuit-
focused, closed-source). We're more comparable to AUnit + EpoxyDuino — but we go further by
making the hardware fakes *behave* (responding to I2C reads, simulating bus transactions)
rather than just stubbing them out.

## The three layers

```
┌──────────────────────────────────────────────────────────┐
│ User's ESP32 sketch (unchanged)                          │
│ digitalWrite(2, HIGH); WiFi.begin(...); Wire.write(...); │
└──────────────────────────────────────────────────────────┘
                       │ compiled against
                       ▼
┌──────────────────────────────────────────────────────────┐
│ platforms/arduino-esp32/ — fake Arduino.h, WiFi.h, ...   │
│ Header-compatible drop-ins, forward to → core/           │
└──────────────────────────────────────────────────────────┘
                       │
                       ▼
┌──────────────────────────────────────────────────────────┐
│ core/ — VirtualClock, EventLog, PinRegistry, I2CBus, ... │
│ Framework-neutral simulation primitives.                 │
└──────────────────────────────────────────────────────────┘
```

The platform adapter (`platforms/arduino-esp32/`) is where `Wire.beginTransmission(0x76)`
gets translated into core-level "I2C bus 0, address 0x76, start a write transaction." The
core (`core/`) doesn't know about Arduino — it knows about I2C as a protocol. That separation
is what lets a future `platforms/esp-idf/` plug in alongside.

## The two test surfaces

We expose two ways to write tests against the same simulator state:

| Surface | Test layer | When to use |
|---|---|---|
| **Unity in-process** (`harness/unity/`) | unit | "after `setup()`, GPIO 2 is HIGH within 500 ms" |
| **pytest-embedded out-of-process** (`harness/pytest_pio_emulator/`, T2+) | scenario | "device boots → connects WiFi → fetches HTTP → publishes MQTT → recovers from disconnect" |

The pytest-embedded surface plugs into Espressif's existing pytest-embedded ecosystem. The
same test code that drives Wokwi or real hardware drives our sim — that's the leverage.

## Tier roadmap (paraphrased)

T0 (now) — skeleton. T1 — GPIO/Serial/timing. T2 — I2C/SPI/ADC/PWM/timers, plus the pytest
plugin. T3 — WiFi, HTTP, MQTT. T4 — filesystem, NVS, deep-sleep, RTOS, BLE.

Each tier ships standalone with verifiable acceptance against the operator's two real
test-candidate projects (`iot-yc-water-the-flowers` and its mini fork).

## Read next

- The [master design spec](../../superpowers/specs/2026-05-05-esp32-pio-emulator-master-design.md)
  for the canonical version of all of the above.
- The current tier's spec under `docs/superpowers/specs/`.
- The current tier's plan under `docs/superpowers/plans/`.
- ADRs under [`../../decisions/`](../../decisions/) for *why* specific choices were made.
```

- [ ] **Step 2: Verify and commit**

```bash
test -f docs/dev/explanation/architecture-overview.md && \
  grep -q "compiled against" docs/dev/explanation/architecture-overview.md && \
  echo "OK"

git add docs/dev/explanation/architecture-overview.md
git commit -m "$(cat <<'COMMIT'
docs(dev): architecture overview

T0 step 6. Short prose tour of the three-layer architecture (sketch /
platform-adapter / core) and two test surfaces (Unity / pytest-embedded)
for contributors who want orientation without reading the full master
spec. Links out to the canonical docs.
COMMIT
)"
```

---

## Task 7: Setting-up-dev-environment guide

**Files:**
- Create: `docs/dev/tutorials/setting-up-dev-environment.md`

- [ ] **Step 1: Create the file**:

```markdown
# Setting up your development environment

This tutorial gets you from a fresh clone to "I can run the framework's own tests" in
under 10 minutes.

## Prerequisites

- Linux (Ubuntu 22.04+) or macOS (13+). Windows is untested — try WSL2 if you must.
- Python 3.10+
- A C++ compiler. On Ubuntu: `sudo apt install build-essential`. On macOS: `xcode-select --install`.

## 1. Install PlatformIO

PlatformIO is the build system for the framework's own tests as well as for users adopting
us downstream.

```bash
python3 -m pip install --user platformio
```

Verify: `pio --version` should print a version string.

## 2. Clone the repo

```bash
git clone https://github.com/fresh-fx59/esp32-pio-emulator
cd esp32-pio-emulator
```

## 3. Run the test suite

```bash
pio test -e native
```

Expected at the end of T0: at least one test passes (the skeleton smoke test). As tiers
ship, the suite grows.

## 4. (Optional) Set up pre-commit hooks

We use the [pre-commit](https://pre-commit.com/) framework to enforce style locally.

```bash
python3 -m pip install --user pre-commit
pre-commit install
```

Now `git commit` will run `clang-format`, trailing-whitespace fix, end-of-file fix, and YAML
lint before every commit. CI runs the same checks, so a missed local hook is caught before
merge.

## 5. (Optional) Editor setup

The repo ships `.editorconfig` and `.clang-format` files. Install the relevant extension for
your editor (most modern editors honor EditorConfig out of the box).

For VS Code:

- C/C++ extension (Microsoft) — picks up `.clang-format`
- EditorConfig for VS Code — picks up `.editorconfig`

## What's next

- Read [`../explanation/architecture-overview.md`](../explanation/architecture-overview.md)
  for a 10-minute architectural tour.
- Find the active tier's plan under [`../../superpowers/plans/`](../../superpowers/plans/).
- Pick a task from the active plan; ask in an issue or PR before starting if it's substantial.
```

- [ ] **Step 2: Verify and commit**

```bash
test -f docs/dev/tutorials/setting-up-dev-environment.md && echo "OK"

git add docs/dev/tutorials/setting-up-dev-environment.md
git commit -m "$(cat <<'COMMIT'
docs(dev): setting-up-dev-environment tutorial

T0 step 7. Five-step contributor onboarding (PlatformIO install, clone,
run tests, optional pre-commit, optional editor setup). Acceptance:
following this tutorial reaches "1 test passes" in under 10 minutes
from a fresh machine.
COMMIT
)"
```

---

## Task 8: platformio.ini

**Files:**
- Create: `platformio.ini`

- [ ] **Step 1: Create `platformio.ini`** — single `[env:native]` env using Unity, gnu++17, and only our own sources (no external lib_deps yet — those land per-tier):

```ini
; PlatformIO Project Configuration File for esp32-pio-emulator
;
; This repo is a *library* — consumers depend on us via lib_deps in their own
; projects. We don't build firmware here. The only env in this file is `native`,
; for running the framework's own tests on the host.
;
; See AGENTS.md for the project-wide conventions, and
; docs/superpowers/specs/2026-05-05-esp32-pio-emulator-master-design.md
; for the architecture.

[env:native]
platform = native
test_framework = unity

; gnu++17 matches the user's mini-fork project; broadly supported on host compilers.
; -Wall -Wextra catches common bugs early; we explicitly do NOT use -Werror in v0.x
; while the framework's surface is still churning.
build_flags =
    -std=gnu++17
    -Wall
    -Wextra
    -DESP32_PIO_EMULATOR_NATIVE=1

; Test sources live under test/. Framework code under core/, platforms/, harness/
; will be added per-tier (T1+); test_build_src=true so test binaries can pull from
; src/ when we add it later.
test_build_src = true
```

- [ ] **Step 2: Verify PIO recognizes the env**

```bash
pio project config 2>&1 | head -20
```

Expected output should include `env:native` and the `build_flags` we set.

- [ ] **Step 3: Commit**

```bash
git add platformio.ini
git commit -m "$(cat <<'COMMIT'
build: platformio.ini with [env:native]

T0 step 8. Single test environment using Unity + gnu++17. No esp32dev
env in this repo — we're a library, consumers create their own ESP32
envs and depend on us via lib_deps. Build flags include -Wall -Wextra
(but not -Werror while the surface still churns). Per ADR-0003,
ArduinoFake is NOT in lib_deps.
COMMIT
)"
```

---

## Task 9: library.json (PlatformIO library metadata)

**Files:**
- Create: `library.json`

- [ ] **Step 1: Create `library.json`**:

```json
{
  "name": "esp32-pio-emulator",
  "version": "0.0.0",
  "description": "Behavioral simulator for ESP32 firmware. Compile your unmodified Arduino sketch natively against host-side fakes; assert on observable behavior with Unity tests. Tier 0 skeleton — implementation begins at Tier 1 (GPIO).",
  "keywords": [
    "esp32",
    "esp32-s3",
    "arduino",
    "testing",
    "tdd",
    "simulator",
    "emulator",
    "unity",
    "native",
    "host-test"
  ],
  "repository": {
    "type": "git",
    "url": "https://github.com/fresh-fx59/esp32-pio-emulator"
  },
  "authors": [
    {
      "name": "fresh-fx59",
      "email": "fresh.fx59@gmail.com",
      "maintainer": true
    }
  ],
  "license": "MIT",
  "frameworks": ["arduino"],
  "platforms": ["espressif32", "native"],
  "dependencies": [],
  "headers": [
    "esp32sim/esp32sim.h"
  ],
  "export": {
    "include": [
      "core",
      "platforms",
      "harness",
      "peripherals",
      "library.json",
      "LICENSE",
      "README.md"
    ],
    "exclude": [
      "test",
      "examples",
      "docs",
      ".github",
      ".pio"
    ]
  }
}
```

- [ ] **Step 2: Verify the file is valid JSON and `pio package pack` works**

```bash
python3 -c "import json; json.load(open('library.json'))" && echo "JSON OK"
pio package pack 2>&1 | tail -5
```

Expected: `JSON OK`, then a tarball is created (look for "successfully packaged" or `.tar.gz` filename in output).

- [ ] **Step 3: Clean up the tarball (don't commit it)**

```bash
rm -f esp32-pio-emulator-*.tar.gz
ls *.tar.gz 2>/dev/null && echo "TARBALL STILL PRESENT - INVESTIGATE" || echo "CLEAN"
```

Expected: `CLEAN`.

- [ ] **Step 4: Commit**

```bash
git add library.json
git commit -m "$(cat <<'COMMIT'
build: library.json — PlatformIO library metadata

T0 step 9. Declares us as a PlatformIO library compatible with the
arduino framework on espressif32 and native platforms. Exports
core/, platforms/, harness/, peripherals/ (the latter three currently
empty; populated per tier). Headers entry points at esp32sim/esp32sim.h
which lands in T1. Dependencies array is empty per ADR-0003 (no
ArduinoFake; users add their own if they want it).
COMMIT
)"
```

---

## Task 10: First skeleton Unity test (TDD shape)

**Files:**
- Create: `test/test_skeleton/test_skeleton.cpp`

- [ ] **Step 1: Write the failing test first** (it will fail to *compile* because the file doesn't exist):

```bash
mkdir -p test/test_skeleton
```

Create `test/test_skeleton/test_skeleton.cpp`:

```cpp
// test/test_skeleton/test_skeleton.cpp
//
// T0 skeleton smoke test. Proves PlatformIO's native env + Unity work in this
// repo. Intentionally trivial — no framework code is exercised yet. T1 replaces
// this with real GPIO TDD tests.

#include <unity.h>

void setUp(void) {
    // Runs before every test. Empty for now.
}

void tearDown(void) {
    // Runs after every test. Empty for now.
}

void test_skeleton_passes(void) {
    TEST_ASSERT_EQUAL_INT(1, 1);
}

void test_native_env_macro_defined(void) {
    // Sanity check that platformio.ini's build_flags reach the test binary.
#ifndef ESP32_PIO_EMULATOR_NATIVE
    TEST_FAIL_MESSAGE("ESP32_PIO_EMULATOR_NATIVE macro not defined "
                      "— platformio.ini build_flags are not reaching the test binary");
#endif
    TEST_PASS();
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_skeleton_passes);
    RUN_TEST(test_native_env_macro_defined);
    return UNITY_END();
}
```

- [ ] **Step 2: Run the tests to verify they pass**

```bash
pio test -e native 2>&1 | tail -15
```

Expected: output ends with something like `2 Tests 0 Failures 0 Ignored OK`. Exit code 0.

- [ ] **Step 3: Verify exit code**

```bash
pio test -e native; echo "exit_code=$?"
```

Expected: `exit_code=0`.

- [ ] **Step 4: Commit**

```bash
git add test/test_skeleton/test_skeleton.cpp
git commit -m "$(cat <<'COMMIT'
test: skeleton smoke test for [env:native]

T0 step 10. Two trivial Unity tests:
- test_skeleton_passes asserts 1 == 1 (proves Unity macros + the test
  runner compile and execute on the host).
- test_native_env_macro_defined asserts platformio.ini's
  ESP32_PIO_EMULATOR_NATIVE define reaches the test binary (proves
  build_flags are wired correctly).

This is intentionally trivial — no framework code is exercised yet.
T1 replaces these with real GPIO TDD tests against a sim API.

Verified locally: pio test -e native green, exit code 0.
COMMIT
)"
```

---

## Task 11: GitHub repo creation + first push

**Files:**
- Modifies: git remote (creates `origin` pointing at GitHub)

- [ ] **Step 1: Create the GitHub repo via `gh`**

```bash
gh repo create fresh-fx59/esp32-pio-emulator \
  --public \
  --description "Behavioral simulator for ESP32 firmware. Compile your unmodified Arduino sketch natively, assert on observable behavior with Unity tests." \
  --source=. \
  --remote=origin
```

Expected: prints the new repo URL. If the repo already exists, `gh` errors out — in that
case, run `git remote add origin git@github.com:fresh-fx59/esp32-pio-emulator.git` instead.

- [ ] **Step 2: Push `main` and set upstream**

```bash
git push -u origin main
```

Expected: lists all commits pushed; final line `Branch 'main' set up to track ...`.

- [ ] **Step 3: Verify the push by checking remote state**

```bash
gh api repos/fresh-fx59/esp32-pio-emulator/commits --jq '.[0].sha[0:7] + " " + .[0].commit.message' | head -1
```

Expected: prints the SHA and commit subject of the most recent commit (matches `git log -1 --oneline`).

(There is nothing to commit in this task — the work is the remote setup itself.)

---

## Task 12: GitHub Actions CI

**Files:**
- Create: `.github/workflows/ci.yml`

- [ ] **Step 1: Create the workflow file**

```bash
mkdir -p .github/workflows
```

Create `.github/workflows/ci.yml`:

```yaml
name: CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  test:
    name: pio test on ${{ matrix.os }}
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        os: [ubuntu-22.04, macos-13]

    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: "3.11"
          cache: pip

      - name: Install PlatformIO
        run: |
          python -m pip install --upgrade pip
          python -m pip install platformio

      - name: PlatformIO version
        run: pio --version

      - name: Run native tests
        run: pio test -e native

      - name: Validate library.json packs
        run: pio package pack --output /tmp/lib.tar.gz
```

- [ ] **Step 2: Commit and push**

```bash
git add .github/workflows/ci.yml
git commit -m "$(cat <<'COMMIT'
ci: GitHub Actions native-test matrix

T0 step 12. Matrix on ubuntu-22.04 + macos-13, runs pio test -e native
on every push to main and on PRs. Also validates library.json packs
cleanly via pio package pack. fail-fast disabled so we see both OS
results. Python 3.11 with pip cache. Latest actions: checkout@v4,
setup-python@v5.
COMMIT
)"
git push origin main
```

- [ ] **Step 3: Wait for CI to complete and verify green**

```bash
# Wait for the run to start, then watch
sleep 10
gh run list --workflow=ci.yml --limit 1
gh run watch
```

Expected: final status `completed success` for both ubuntu-22.04 and macos-13 jobs.

If CI fails: read `gh run view --log-failed` and fix in a follow-up commit.

---

## Task 13: Pre-commit hooks

**Files:**
- Create: `.pre-commit-config.yaml`

- [ ] **Step 1: Create `.pre-commit-config.yaml`**:

```yaml
repos:
  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v5.0.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-yaml
      - id: check-added-large-files
        args: ['--maxkb=512']
      - id: check-merge-conflict
      - id: mixed-line-ending
        args: ['--fix=lf']

  - repo: https://github.com/pre-commit/mirrors-clang-format
    rev: v18.1.8
    hooks:
      - id: clang-format
        types_or: [c++, c]
```

(`ruff` hook is intentionally NOT here yet — Python lands in T2.)

- [ ] **Step 2: Install pre-commit and run it on all files**

```bash
python3 -m pip install --user pre-commit
~/.local/bin/pre-commit install
~/.local/bin/pre-commit run --all-files
```

Expected: most hooks `Passed`. `trailing-whitespace` or `end-of-file-fixer` may *modify*
files (and re-fail once); rerun until clean. If `clang-format` modifies files, that's
expected on first run — accept its formatting changes and rerun.

- [ ] **Step 3: Stage any pre-commit fixes**

```bash
git status --short
git add -A
git status --short
```

Expected: shows what (if anything) pre-commit fixed; after `git add -A`, working tree is clean of unstaged changes.

- [ ] **Step 4: Verify hooks run on next commit (test it)**

```bash
~/.local/bin/pre-commit run --all-files
```

Expected: all hooks `Passed`. If anything still fails, fix manually and re-run.

- [ ] **Step 5: Commit**

```bash
git add .pre-commit-config.yaml
# include any pre-commit-driven file fixes from step 3
git diff --cached --stat
git commit -m "$(cat <<'COMMIT'
chore: pre-commit hooks (clang-format, whitespace, eof, yaml)

T0 step 13. Standard hygiene hooks plus clang-format for C/C++.
ruff hook is deliberately deferred to T2 when the Python pytest
plugin lands — no Python in the repo yet.

Hook versions pinned. Running pre-commit run --all-files locally
came back clean (or only modified files in ways that match the
configured style).

Same checks run in CI (TODO follow-up: wire pre-commit into the CI
workflow after T0 sign-off so PRs that skip the hook are caught
upstream).
COMMIT
)"
git push origin main
```

---

## Task 14: T0 sign-off — bump CHANGELOG, README status, CLAUDE.md, push final state

**Files:**
- Modify: `CHANGELOG.md`
- Modify: `README.md`
- Modify: `CLAUDE.md`

- [ ] **Step 1: Update `CHANGELOG.md`** — replace the `[Unreleased]` block contents with a real `[0.1.0]` entry:

```markdown
# Changelog

All notable changes to this project will be documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

(Nothing yet.)

## [0.1.0] — 2026-05-05

### Added
- Tier 0 skeleton: project metadata files, AGENTS.md, CLAUDE.md, Diátaxis docs scaffold,
  architecture overview, dev environment setup tutorial.
- `platformio.ini` with `[env:native]` (gnu++17, Unity test framework).
- `library.json` declaring the project as a PlatformIO library.
- First skeleton Unity test (`test/test_skeleton/test_skeleton.cpp`) — green.
- GitHub Actions CI matrix: Ubuntu 22.04 + macOS 13.
- Pre-commit hooks: clang-format + standard hygiene.

### Fixed
- N/A (first release).

## [0.0.0] — 2026-05-05

### Added
- Project conceived. Master design spec and tier specs T0–T4 written and committed.
- ADR-0001 (ESP32-S3 primary target) and ADR-0003 (no ArduinoFake coexistence) accepted.
```

- [ ] **Step 2: Update `README.md`** — flip the T0 row from 🚧 to ✓:

In `README.md`, find:

```markdown
| T0 | Skeleton (this tier) | 🚧 in progress |
```

Replace with:

```markdown
| T0 | Skeleton | ✓ shipped 2026-05-05 |
| T1 | GPIO TDD | 🚧 next |
```

(And remove the now-redundant T1 ⏸ row — keep T2/T3/T4 ⏸ rows unchanged.)

Also update the status badge at the top from `Status: **Tier 0 in progress**` to:

```markdown
> Status: **Tier 1 starting** · License: MIT · ESP32-S3 primary target · gnu++17
```

- [ ] **Step 3: Update `CLAUDE.md`** — flip current state:

In `CLAUDE.md`, find the "## Current state" block and replace with:

```markdown
## Current state

- **Active tier:** T1 (GPIO TDD) — starting. Refresh T1 spec, then write T1 plan.
- **Last shipped tier:** T0 skeleton, sign-off 2026-05-05. CHANGELOG entry [0.1.0].
- **Last verified:** `pio test -e native` green on Ubuntu + macOS in CI.

See [`docs/superpowers/specs/2026-05-05-tier-1-gpio-tdd-design.md`](docs/superpowers/specs/2026-05-05-tier-1-gpio-tdd-design.md)
for the T1 spec. Per the spec-drift policy, refresh it before writing the T1 plan.
```

- [ ] **Step 4: Run the consumer-side smoke test** (acceptance criterion #4 from T0 spec)

This is the load-bearing acceptance proof: a *separate* PlatformIO project pulling us in via `lib_deps` and running native tests sees our skeleton test pass.

```bash
SCRATCH=$(mktemp -d)
cd "$SCRATCH"
mkdir -p test/test_consumer
cat > platformio.ini <<'EOF'
[env:native]
platform = native
test_framework = unity
lib_deps =
    https://github.com/fresh-fx59/esp32-pio-emulator
build_flags = -std=gnu++17
EOF
cat > test/test_consumer/test_consumer.cpp <<'EOF'
#include <unity.h>
void setUp(void) {}
void tearDown(void) {}
void test_consumer_can_use_library(void) { TEST_ASSERT_TRUE(true); }
int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_consumer_can_use_library);
    return UNITY_END();
}
EOF
pio test -e native; echo "consumer_smoke_exit=$?"
cd /home/claude-developer/esp32-pio-emulator
rm -rf "$SCRATCH"
```

Expected: `consumer_smoke_exit=0`. The library installs from GitHub, the consumer's test compiles and runs. (Our own skeleton tests under `test/test_skeleton/` are *not* run in the consumer scratch project — that's by design; consumers run their own tests, ours stay home.)

If this fails, the most likely cause is `library.json`'s `export.include` not matching the actual layout — fix and recommit, repeat.

- [ ] **Step 5: Verify the full T0 acceptance gate**

Run all seven deliverable checks from the T0 spec:

```bash
echo "=== Deliverable 1: tree matches master spec §6 ==="
ls -la | grep -E "AGENTS\.md|CLAUDE\.md|README\.md|LICENSE|CHANGELOG|platformio\.ini|library\.json|\.clang-format|\.editorconfig|pyproject\.toml|\.pre-commit-config\.yaml|docs|test|\.github" | wc -l

echo "=== Deliverable 2: pio test -e native ==="
pio test -e native; echo "  exit_code=$?"

echo "=== Deliverable 3: pio package pack ==="
pio package pack --output /tmp/lib.tar.gz && tar -tzf /tmp/lib.tar.gz | grep -q library.json && echo "  OK"

echo "=== Deliverable 4: consumer-side smoke (already verified in step 4 above) ==="

echo "=== Deliverable 5: CI green ==="
gh run list --workflow=ci.yml --limit 1 --json status,conclusion --jq '.[0]'

echo "=== Deliverable 6: docs/README.md links resolve ==="
grep -oE 'docs/[a-z/]+/' docs/README.md | sort -u | while read p; do
  test -d "$p" && echo "  OK: $p" || echo "  MISSING: $p"
done

echo "=== Deliverable 7: AGENTS.md has no MPA-specific facts ==="
grep -c "MPA\|cliproxyapi\|OVH\|Contabo\|Telegram\|45\.151\.30\.146\|31\.220\.78\.216" AGENTS.md
```

Expected: every check passes; deliverable 7 prints `0`.

- [ ] **Step 6: Commit + push T0 sign-off**

```bash
git add CHANGELOG.md README.md CLAUDE.md
git commit -m "$(cat <<'COMMIT'
release: tier 0 (skeleton) shipped, v0.1.0

T0 step 14 — sign-off. All seven deliverables green:
- Tree matches master spec §6.
- pio test -e native: 2 tests, 0 failures, exit 0.
- pio package pack produces a valid tarball.
- Consumer-side smoke test: a scratch PIO project lib_deps-ing us
  via the GitHub URL ran its own test green (proves we work as a
  library to downstream users).
- CI green on Ubuntu 22.04 + macOS 13.
- docs/README.md links resolve to all eight Diátaxis subfolders.
- AGENTS.md contains zero MPA-specific facts.

CHANGELOG bumped 0.0.0 -> 0.1.0. README status table flipped T0 ✓ /
T1 🚧. CLAUDE.md current-state updated to "T1 starting".

Next: refresh T1 spec, then write T1 plan via writing-plans skill.
COMMIT
)"
git push origin main
```

---

## Self-review

Run the spec self-review check from the writing-plans skill against this plan:

**1. Spec coverage** — every deliverable in the [T0 spec's deliverables checklist](../specs/2026-05-05-tier-0-skeleton-design.md) maps to a task here:

| Spec deliverable | Plan task |
|---|---|
| Tree matches master spec §6 | T1, T5 (Diátaxis), T8, T9 (PIO files), and verified in T14 |
| `pio test -e native` runs and reports 1+ test passing | T10 |
| `pio package pack` produces a valid library tarball | T9 step 2, verified in T14 |
| Consumer-side smoke test | T14 step 4 |
| CI green on a no-op PR | T12 step 3 |
| `docs/README.md` resolves correctly to all subfolders | T5 step 2 |
| `AGENTS.md` lints (no MPA-specific leakage) | T2 step 2, verified in T14 |

**2. Placeholder scan** — search for `TBD`, `TODO`, `FIXME`, `XXX`, `placeholder` in this plan: only acceptable matches are inside *file content* shown to be created (e.g., subfolder README files describe what's *coming in T1*, which is intentional, not a plan placeholder). No "fill in details" or "implement later" anywhere in plan steps.

**3. Type/identifier consistency** — file paths, hook names, env names match across tasks: `[env:native]` everywhere; `test/test_skeleton/test_skeleton.cpp`; `.pre-commit-config.yaml`; CHANGELOG/README/CLAUDE.md updates in T14 reference exactly the strings created in T1/T3.

**4. AGENTS.md / spec compliance:**
- Subagent-driven plan execution? Plan structured as 14 discrete tasks. ✓
- Per-step verify-and-document? Every task ends with verification command + commit. ✓
- Small reviewable commits? One concept per task, one commit per task (or tight mini-series). ✓
- Spec-drift discipline? Plan doesn't *change* the spec — if reality contradicts the plan during execution, executor is expected to update the spec first per AGENTS.md. ✓

If issues are found during execution, the executor logs them, updates the spec or this plan, and continues. Do not block on perfectionism.

---

## Execution handoff

Plan complete and saved to `docs/superpowers/plans/2026-05-05-tier-0-skeleton.md`.

Per [AGENTS.md](../../../AGENTS.md) Execution defaults — *Plan execution mode: subagent-driven* — execute via `superpowers:subagent-driven-development`: dispatch one fresh subagent per task with a review checkpoint between tasks. Do not ask the operator to choose execution mode; proceed unless they explicitly override.
