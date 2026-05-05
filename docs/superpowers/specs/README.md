# Specs

Design specs for `esp32-pio-emulator`, ordered by dependency.

| File | Status | What's in it |
|---|---|---|
| [`2026-05-05-esp32-pio-emulator-master-design.md`](2026-05-05-esp32-pio-emulator-master-design.md) | Draft v0.1 | Mission, architecture, core abstractions, repo layout, tier roadmap, open decisions, glossary |
| [`2026-05-05-tier-0-skeleton-design.md`](2026-05-05-tier-0-skeleton-design.md) | Draft v0.1 — implementation-ready | Repo bootstrap, AGENTS.md, CLAUDE.md, Diátaxis docs scaffold, PlatformIO project, GH Actions CI |
| [`2026-05-05-tier-1-gpio-tdd-design.md`](2026-05-05-tier-1-gpio-tdd-design.md) | Draft v0.1 — implementation-ready | GPIO, Serial, virtual time, `attachInterrupt`; first end-to-end TDD loop |
| [`2026-05-05-tier-2-sensor-tdd-design.md`](2026-05-05-tier-2-sensor-tdd-design.md) | Draft v0.1 — detailed | I2C, SPI, ADC, PWM, hardware timers; first stateful peripheral fakes; pytest-embedded plugin α |
| [`2026-05-05-tier-3-networked-design.md`](2026-05-05-tier-3-networked-design.md) | Architectural sketch — rewrite at T3 entry | WiFi, HTTP, MQTT, NTP, mDNS; networking impl decision |
| [`2026-05-05-tier-4-full-chip-design.md`](2026-05-05-tier-4-full-chip-design.md) | Architectural sketch — rewrite at T4 entry | Filesystem, NVS, deep-sleep, RTOS, RMT, BLE, BT |

## Reading order

1. Start with the **master design**.
2. Read the tier you're about to work on. (T0 first if implementing from scratch.)
3. Skim later tiers only for context — they will be rewritten before they're implemented.

## Spec-drift discipline (per AGENTS.md)

If reality contradicts a spec, **commit a spec update first**, then adapt the implementation. Do not let stale specs become decoration. T3 and T4 in particular are explicitly written as sketches and **must** be rewritten as v0.2 before their implementation begins.

ADRs (architectural decision records) live in `../../decisions/`.
