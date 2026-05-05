# Tier 4 — Full chip

| | |
|---|---|
| **Status** | Architectural sketch — most decisions deferred until prior tiers ship |
| **Date** | 2026-05-05 |
| **Parent** | [Master design](2026-05-05-esp32-pio-emulator-master-design.md) |
| **Depends on** | [Tier 3](2026-05-05-tier-3-networked-design.md) shipped |
| **Confidence** | Low — this spec is a roadmap, not an implementation guide. **Mandatory rewrite at T4 entry.** |

> **Read this first.** This is the most speculative spec in the set. By the time T4 begins, T1–T3 will have taught us a lot, and this document will look naïve in places. **Per AGENTS.md spec-drift policy, T4 spec is rewritten as v0.2 before T4 implementation begins.** What's here is the *intent* and the *acceptance shape*, not the precise plan.

## Goal

A non-trivial real-world ESP32 project — a temperature logger with deep-sleep, BLE provisioning, WiFi MQTT publishing, NVS persistence, and a small filesystem — runs end-to-end in CI without any hardware. After T4, the simulator covers the practical surface area of typical IoT-device firmware.

## Scope

### In — capability areas

1. **Filesystem (LittleFS, SPIFFS)** — open / read / write / delete / mkdir / rmdir / readdir. Backed by an in-memory file tree. Test API can pre-seed files and snapshot post-test state.
2. **NVS / Preferences** — key-value store with type-tagged values. In-memory backend; test API can pre-seed or snapshot.
3. **Deep sleep & wake reasons** — `esp_deep_sleep_start()` doesn't exit; the sim re-runs `setup()` with the configured wake reason. Test API drives this. Wake-from-EXT0/EXT1/timer/touchpad all simulable.
4. **RMT (Remote Control Transceiver)** — pulse-train I/O. Used for IR LEDs, WS2812B/NeoPixel, etc.
5. **FreeRTOS / dual-core scheduling** — *the big one*. See §3 below.
6. **Bluetooth Classic** — pairing, SPP. (Lower priority; modern ESP32 IoT projects rarely use it.)
7. **BLE (Bluetooth Low Energy)** — `BLEDevice`, advertising, GATT server / client, characteristics. Common for provisioning flows. *This is the second-biggest item after RTOS.*
8. **ULP coprocessor** (?) — if scope allows. May defer to T4.5.
9. **Power-mode transitions** — light sleep vs deep sleep, wakeup latencies.

### Explicitly Not In Scope, Even In T4
- Cryptographic accelerator hardware fidelity (we use host crypto libs).
- Hardware crypto fuses / eFuses.
- Wi-Fi/BT coexistence timing fidelity.
- Real radio behavior (range, interference).
- Cycle-accurate dual-core memory contention.

## The two big T4 decisions

### D2 — RTOS implementation

Three options, decide at T4 entry as an ADR:

| Option | What it is | Pros | Cons |
|---|---|---|---|
| **(a) Cooperative pseudo-scheduler** | Tasks yield at API boundaries (`vTaskDelay`, queue ops, semaphore takes). No real preemption. | Simple, deterministic, fast tests. | Can't reproduce true preemption races. |
| **(b) FreeRTOS POSIX port** | Use the upstream FreeRTOS POSIX port — actual FreeRTOS code running on Pthreads. | Behaviorally faithful — real preemption, real priorities, real queues. | Test determinism harder; flake risk; debugger story trickier. |
| **(c) Hybrid** | Default cooperative; opt-in real-FreeRTOS mode for tests that explicitly want preemption. | Best of both. | Two code paths to maintain. |

**Tentative recommendation (not committed):** start with (a). Most firmware bugs aren't preemption races. Add (c)'s opt-in real-mode if user feedback demands it. (b) is too disruptive to test ergonomics for v1.

### D7 (new) — BLE simulation depth

| Option | Description |
|---|---|
| **(a) Stub-level** | `BLEDevice::init`, `advertising`, `addService` recorded as events. Tests assert on what was advertised. No actual BLE peer behavior. |
| **(b) Behavioral** | Fake BLE peer in Python harness; can connect, read characteristics, write characteristics, get notifications. Sketch sees a real-ish BLE central / peripheral. |

Option (a) is a 1-week thing; option (b) is a 1-month thing. Likely we ship (a) at T4 entry and add (b) as T4.5 if BLE provisioning testing matters enough.

## Implementation sketch (illustrative — DO NOT take literally)

(The real step list lands during T4 brainstorming, after T3 ships. What's below is a directional outline so the master spec's tier table has substance behind it.)

1. ADR — RTOS impl choice.
2. ADR — BLE depth choice.
3. `core/Filesystem` + LittleFS / SPIFFS adapters.
4. `core/Nvs` + Preferences adapter.
5. `core/PowerMgmt` + deep-sleep semantics. **This rewires `runSetup()` / `runLoop()` flow** — sketch can now exit to deep-sleep and "wake" with a wake reason and persisted NVS.
6. RTOS shim implementation (per ADR).
7. RMT.
8. BLE (per ADR depth).
9. Bluetooth Classic.
10. End-to-end reference project: `examples/05-deep-sleep-ble-provision/` — temperature logger with BLE provisioning, NVS-stored credentials, deep-sleep cycles, WiFi MQTT publish on wake.
11. Comprehensive docs sweep — at this point the user-facing API surface is huge and the docs need a refresh.
12. v1.0 release.

## Verification gate (Tier 4 acceptance — and v1.0)

T4 / v1.0 ships when:

- The reference project (`05-deep-sleep-ble-provision`) runs end-to-end in CI, fully offline, including:
  - First-boot detects no NVS credentials.
  - BLE peer (Python fixture) connects and provisions WiFi credentials.
  - Sketch saves credentials to NVS, reboots.
  - Sketch wakes with credentials in NVS, connects to WiFi, reads BMP280, publishes to MQTT, deep-sleeps.
  - Sketch wakes again from deep-sleep timer; repeats publish; deep-sleeps.
- All four tiers' reference examples remain green (no regressions).
- Coverage of `core/` ≥ 80%, `peripherals/` ≥ 70%, `platforms/` ≥ 80%.
- Comprehensive documentation: each major capability has tutorial + how-to + reference.
- v1.0 announcement post (`docs/explanation/v1-launch.md`?). PlatformIO library registry listing live.

## Documentation deliverables

- `docs/user/how-to/test-deep-sleep.md`
- `docs/user/how-to/test-ble-provisioning.md`
- `docs/user/how-to/test-with-littlefs.md`
- `docs/user/how-to/test-rtos-tasks.md`
- `docs/user/reference/full-api-coverage.md` — what's supported, what's not.
- `docs/user/explanation/dual-core-and-rtos-fidelity.md`
- `docs/decisions/00NN-rtos-impl.md`, `00NN+1-ble-depth.md`.

## Open questions (T4-specific, all defer until T4 entry)

- **Q-T4-1 (D2):** RTOS impl — (a), (b), or (c)?
- **Q-T4-2 (D7):** BLE depth — stub or behavioral?
- **Q-T4-3:** Do we simulate ULP coprocessor? *Default: no in T4 v1.0; defer to v1.1 if demand.*
- **Q-T4-4:** Touch sensor simulation? *Default: thin behavioral fake — sufficient for typical wake-from-touch use.*
- **Q-T4-5:** Hall sensor? *Default: stub.*
- **Q-T4-6:** Camera (esp32-cam variants)? *Default: stub; full sim is a v2 project.*
- **Q-T4-7:** ESP32 variant matrix — by T4 we should know which variants we support (S2/S3/C3/C6/H2). *Default decision: support the common subset of peripherals across S2/S3/C3, treat C6/H2 as best-effort.*

## What this tier still won't catch (the honest list)

- Real-world dual-core memory ordering / cache coherency bugs (unless we picked RTOS option b).
- Real RF interference (BT/WiFi coexist).
- Brownout, voltage glitches, power supply effects.
- Bootloader / partition table bugs.
- OTA update edge cases (partial flash, failed verify).
- Cycle-accurate timing (e.g., bit-banged protocols' exact pulse widths).

These remain reasons to test on real hardware before shipping. The simulator catches a large fraction of firmware bugs; it does not replace hardware QA.
