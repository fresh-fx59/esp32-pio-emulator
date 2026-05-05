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

## Project scope answers

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
