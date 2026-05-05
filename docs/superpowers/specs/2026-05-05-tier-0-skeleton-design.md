# Tier 0 — Skeleton

| | |
|---|---|
| **Status** | Draft v0.1 — pending user review |
| **Date** | 2026-05-05 |
| **Parent** | [Master design](2026-05-05-esp32-pio-emulator-master-design.md) |
| **Confidence** | High — implementation-ready |

## Goal

Stand up the empty repository with everything Tier 1 needs to begin, *without writing any simulator code*. After Tier 0, a contributor can `git clone`, `pio test -e native`, and see a single trivial test pass on Linux, macOS, and in CI.

The aim is to make every subsequent tier's first step "add behavior to a working scaffold" instead of "build the scaffold first."

## Scope

### In
- Git repository, branch policy, `.gitignore`, `LICENSE` (MIT).
- `AGENTS.md` adapted from the user's MPA template — keep execution defaults / docs currency / safety; replace MPA-specific tables with esp32-pio-emulator equivalents (or remove if not yet relevant).
- `CLAUDE.md` — task-handoff context for future sessions.
- `README.md` — what this is, who it's for, status badge, "5-second pitch", pointer to docs.
- `docs/` Diátaxis scaffold + `docs/README.md` map. All folders exist with a `.gitkeep` or a placeholder `README.md`.
- `platformio.ini` with one environment: `native` (where our tests will run, using Unity). The repo *is* a PlatformIO library, not an application — we don't build firmware here. `library.json` declares compatibility with the `espressif32` platform / `arduino` framework so consumers can `lib_deps` us into their projects.
- `library.json` so this repo can be installed as a PlatformIO library.
- A single trivial Unity test under `test/test_skeleton/test_skeleton.cpp` that asserts `1 == 1`. No simulator code is exercised yet — this just proves the toolchain works.
- GitHub Actions CI: matrix on `ubuntu-latest` and `macos-latest`, runs `pio test -e native`.
- Code style files: `.clang-format`, `.editorconfig`, `pyproject.toml` for Python tooling stubs (even though no Python lands until T2 — the file exists so settings are durable).
- A `CHANGELOG.md` seeded with `0.0.0 — repo bootstrap`.

### Out (deferred to later tiers)
- Any fake `Arduino.h` or peripheral implementation — that is Tier 1.
- The pytest-embedded plugin — Tier 2.
- Examples beyond a placeholder `examples/README.md`.
- Logo, website, marketing copy.

## Deliverables checklist

A contributor on a fresh clone should observe these as outcomes:

| # | Deliverable | Verification |
|---|---|---|
| 1 | `git clone` produces a tree matching the master spec §6 | `tree -L 2` |
| 2 | `pio test -e native` runs and reports 1 test passing | exit code 0; output `1 Tests 0 Failures` |
| 3 | `pio package pack` produces a valid library tarball | exit code 0; tarball contains `library.json` |
| 4 | A consumer project that adds `esp32-pio-emulator` to `lib_deps` and runs `pio test -e native` sees 1 test pass (consumer-side smoke test) | done from a separate scratch dir |
| 5 | CI green on a no-op PR | GH Actions ✓ |
| 6 | `docs/README.md` resolves correctly to all subfolders | manual click-through |
| 7 | `AGENTS.md` lints (links resolve, no leftover MPA-specific facts that don't apply here) | manual review |

## Implementation steps

Each step is one commit, ends with the verification command in its commit body, and updates docs in the same commit per AGENTS.md doctrine.

1. **Initial commit — `.gitignore`, `LICENSE`, empty `README.md`.** Verify: `git log` shows one commit; `cat LICENSE` shows MIT.
2. **`AGENTS.md` adapted from MPA template.** Keep §Execution defaults, §Documentation currency, §Safety. Replace §β scope / §Hosts / §Upstream / §cliproxyapi quirks / §Telegram test account with esp32-pio-emulator equivalents (most empty for now — placeholder tables labelled "T.B.D. when first applicable"). Verify: read top-to-bottom, no MPA-specific facts leak.
3. **`CLAUDE.md`** — what an agent picking up this repo cold needs to know: project mission (one paragraph), current tier, link to the master spec, link to the active per-tier spec, what *not* to touch yet. Verify: a fresh agent can orient in <2 minutes.
4. **`docs/` Diátaxis scaffold + `docs/README.md` map.** All eight subfolders (`user/{tutorials,how-to,reference,explanation}`, `dev/{tutorials,how-to,reference,explanation}`) exist with a `README.md` describing what goes there. `docs/decisions/` and `docs/superpowers/specs/` exist (the latter already has the spec files). Verify: `find docs -name README.md` lists all expected paths.
5. **`platformio.ini` with `native` env only.** Uses `test_framework = unity`. Verify: `pio project config` shows the `native` env. Document why no `esp32dev` env: this repo is a library, consumers create their own ESP32 envs and depend on us via `lib_deps`.
6. **`library.json` minimal.** `name`, `version`, `description`, `keywords`, `repository`, `frameworks`, `platforms` filled. Verify: `pio package pack` produces a valid tarball.
7. **`test/test_skeleton/test_skeleton.cpp`** with one Unity test (`TEST_ASSERT_EQUAL_INT(1, 1)`). Verify: `pio test -e native` reports 1 passing test.
8. **`.github/workflows/ci.yml`** matrix on Ubuntu + macOS, runs `pio test -e native`. Push branch, open PR against `main`, verify green.
9. **`.clang-format`, `.editorconfig`, `pyproject.toml`.** Conventional defaults (LLVM style, 100 cols, UTF-8, LF). Verify: `clang-format --version` succeeds locally and the format applied to the test file is stable (`clang-format -i && git diff` is empty).
10. **Tier 0 sign-off commit:** updates `CHANGELOG.md` to `0.1.0 — Tier 0 skeleton`, updates `README.md` status badge to "Tier 0 ✓", updates `CLAUDE.md` "current tier" to "Tier 1 starting". Verify: all checks in the deliverables table above pass.

## Documentation deliverables

Every doc that exists at the end of T0:

- `README.md` (root) — pitch, status, install hint (T1+), pointer to `docs/`.
- `AGENTS.md` (root) — durable preferences.
- `CLAUDE.md` (root) — task handoff.
- `CHANGELOG.md` (root).
- `docs/README.md` — Diátaxis map.
- `docs/dev/tutorials/setting-up-dev-environment.md` — how a contributor sets up to work on the framework. PlatformIO install, Python toolchain (deferred actual use to T2 but document setup now), `pre-commit` hook setup.
- `docs/dev/explanation/architecture-overview.md` — short prose version of master spec §4 + §5, for contributors who don't want to read the whole spec.
- `docs/superpowers/specs/2026-05-05-*-design.md` — already exist (this set of files).
- One `.gitkeep` or placeholder `README.md` in every other Diátaxis subfolder so the structure is discoverable.

## Acceptance gate

Tier 0 is "shipped" when:

1. All 10 implementation steps committed and pushed.
2. CI is green on `main`.
3. A contributor on a clean machine, following `docs/dev/tutorials/setting-up-dev-environment.md`, reaches "1 test passes" in under 10 minutes.
4. `CHANGELOG.md` reflects the tag.
5. The Tier 1 spec is reviewed and ready (this happens during T0 wrap-up — not blocking T0 acceptance, but expected to fall out naturally).

## Open questions (T0-specific)

- **Q-T0-1:** Do we want a `pre-commit` hook framework (`pre-commit.com`) wired in T0, or defer to T1? *Default: wire it in T0 with `clang-format` and `trailing-whitespace` hooks. Add more as we grow.*
- **Q-T0-2:** Do we ship a `Dockerfile` for reproducible CI? *Default: no, GH Actions runners are sufficient for now. Revisit if CI flakiness emerges.*
- **Q-T0-3:** Linux distros to support? *Default: Ubuntu 22.04+ LTS, macOS 13+. Document Windows as "should work via WSL2, untested" until someone actually tries.*

These are operator-decidable in autonomous mode per AGENTS.md "decide-under-uncertainty"; defaults shown are what we'll proceed with unless overridden.
