# Changelog

All notable changes to this project will be documented in this file. The format is based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to
[Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

(Nothing yet — Tier 1 (GPIO TDD) is next.)

## [0.1.0] — 2026-05-05

### Added
- Tier 0 skeleton: project metadata files, AGENTS.md, CLAUDE.md, Diátaxis docs scaffold,
  architecture overview, dev environment setup tutorial.
- `platformio.ini` with `[env:native]` (gnu++17, Unity test framework).
- `library.json` declaring the project as a PlatformIO library.
- First skeleton Unity test (`test/test_skeleton/test_skeleton.cpp`) — green.
- GitHub Actions CI on Ubuntu 22.04. (macOS-13 in CI deferred — see master spec D12.)
- Pre-commit hooks: clang-format + standard hygiene.
- ADR-0002 (superseded by ADR-0003) recorded in `docs/decisions/` for posterity.

### Notes
- macOS-13 was originally in the CI matrix but was dropped during T0 implementation
  because free GitHub Actions macOS runners queued 40+ minutes without starting. macOS
  remains a supported OS for contributors; CI coverage returns when there is
  platform-sensitive code to test.
- System PIO 4.3.4 on Ubuntu 24/Python 3.12 is broken (Click API incompatibility); the
  dev setup tutorial uses a project-local `.venv/` for PIO 6.x.

## [0.0.0] — 2026-05-05

### Added
- Project conceived. Master design spec and tier specs T0–T4 written and committed.
- ADR-0001 (ESP32-S3 primary target) and ADR-0003 (no ArduinoFake coexistence) accepted.
