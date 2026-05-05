# Tier 3 — Networked ESP32

| | |
|---|---|
| **Status** | Architectural sketch — to be sharpened at T3 entry |
| **Date** | 2026-05-05 |
| **Parent** | [Master design](2026-05-05-esp32-pio-emulator-master-design.md) |
| **Depends on** | [Tier 2](2026-05-05-tier-2-sensor-tdd-design.md) shipped, including pytest-embedded plugin |
| **Confidence** | Lower — many decisions deferred. Expect to spec-update before implementation. |

> **Read this first.** This document captures the *shape* of T3, not the implementation. Several core architectural decisions (most notably the networking impl approach) are deliberately deferred — see §4 below. **Per AGENTS.md spec-drift policy, this spec gets rewritten via a v0.2 commit before T3 implementation begins.** Treat the v0.1 numbers and module names as illustrative, not load-bearing.

## Goal

A sketch that connects to WiFi, fetches an HTTP resource, and publishes to MQTT runs end-to-end **offline, in CI, in milliseconds**. The same sketch source runs unchanged against real hardware, Wokwi, QEMU, and our sim.

## Scope

### In — APIs that work behaviorally

| Arduino / ESP32 API | T3 behavior |
|---|---|
| `WiFi` (`WiFiClass`) | `begin(ssid, pass)`, `disconnect()`, `status()`, `localIP()`, `RSSI()`. State machine: IDLE → CONNECTING → CONNECTED → DISCONNECTED. Test API forces transitions. |
| `WiFiClient` | `connect(host, port)`, `write`, `read`, `available`, `connected`, `stop`. TCP semantics. |
| `WiFiServer` | `begin(port)`, `available()`. |
| `WiFiUDP` | `begin`, `beginPacket`, `endPacket`, `write`, `read`. |
| `HTTPClient` | `begin`, `GET`, `POST`, `getString`, headers. Backed by an HTTP-level mock or a real socket to `pytest-httpserver`. |
| `PubSubClient` (Nick O'Leary, the de-facto MQTT lib) | `connect`, `publish`, `subscribe`, `loop`. Backed by an embedded MQTT broker fixture (Python side). |
| `WebServer` | `on(path, handler)`, `begin`, `handleClient`. |
| `NTPClient` (a popular community lib) | `update`, `getEpochTime`. Backed by virtual clock + a stub NTP server. |
| `mDNS` | `begin`, `addService`. Discovery in test scope. |

### Out (still deferred)
- Filesystem, NVS, deep-sleep, RTOS — Tier 4.
- BLE — Tier 4.
- Bluetooth Classic — Tier 4.
- ESP32 power-management states — Tier 4.

## The big T3 decision (D1 from master spec)

How does the sim implement networking? Two paths, decide at T3 entry as an ADR:

### Path A — Python mock servers (recommended starting point)

The sim's `WiFiClient::connect(host, port)` forwards (via the control protocol) to the Python harness, which routes to a fixture-provided endpoint:
- HTTP traffic → `pytest-httpserver` instance.
- MQTT traffic → an embedded broker (e.g., `aiomqtt-broker` or `hbmqtt`) running in-process in pytest.
- Raw TCP → a Python `asyncio` server fixture.

**Pros:** Reuses mature Python testing tools. Trivial to assert "the device made a GET to /api/v1/status with header X". Zero networking code in C++. Good debug story.

**Cons:** Sketch can't be exercised by Unity-only (in-process) tests for networking — these scenarios always require pytest-embedded. WiFi state machine still simulated in C++; only payload routing is Python-side.

### Path B — Real LWIP on host (mireq/esp32-simulator pattern)

Sketch links against real LWIP compiled for host. Tests provide tap devices or in-process socket pairs. WiFi is a virtual radio that bridges to a Python-side simulated AP.

**Pros:** Most behaviorally faithful — real TCP retransmits, real DNS, real socket semantics. In-process Unity tests can do networking too.

**Cons:** Significantly more implementation complexity. Linux-leaning (tap devices). Slower test feedback. mireq's project shows this is doable but they've been at it for years and it still has limited peripheral coverage.

### Recommendation (placeholder, not committed)

**Start Path A; reserve Path B for a future T3.5 if real-LWIP fidelity becomes blocker.** Path A gets us a usable T3 in months instead of a year, and Python-side mocks are the *normal* way networked-device tests are written in the embedded world.

This recommendation is **not** the final ADR. The ADR happens at T3 entry, after we've actually built T1 + T2 and know more.

## Implementation sketch (illustrative)

(Treat as a roadmap, not a step list. The real step list will be written during T3 brainstorming.)

1. WiFi state machine in `core/WifiState`. Test API to force transitions. Events emitted on every change.
2. Decision ADR: Path A vs B (or hybrid). Land the ADR commit before code.
3. (Path A) `core/NetworkShim` interface; `harness/pytest_pio_emulator/` adds HTTP/MQTT/TCP fixture managers.
4. `platforms/arduino-esp32/WiFi`, `WiFiClient`, `WiFiServer`, `WiFiUDP` — backed by `NetworkShim`.
5. `platforms/arduino-esp32/HTTPClient` (Espressif's library — since it's part of arduino-esp32's stdlib).
6. PubSubClient — note this is a third-party library; we don't ship a fake of it. We provide an MQTT broker fixture so the real `PubSubClient` works against it.
7. NTP, mDNS.
8. End-to-end example: `examples/06-mqtt-temperature/` — BMP280 → WiFi → MQTT publish.
9. Documentation, CHANGELOG, sign-off.

## Verification gate (Tier 3 acceptance)

T3 ships when:

- A sketch using `WiFi`, `HTTPClient`, and `PubSubClient` runs end-to-end in pytest-embedded, performs a complete connect-fetch-publish cycle, and the test asserts on:
  - WiFi state transitions in the right order.
  - HTTP request hitting the test server with expected headers.
  - MQTT message published to the test broker with expected payload.
- Disconnection / reconnection scenarios are testable (forced disconnect → sketch reconnects).
- Documentation: `docs/user/how-to/test-a-wifi-sketch.md`, `docs/user/how-to/test-mqtt.md`, `docs/decisions/0002-tier-3-networking-impl.md`.
- Coverage targets met (≥75% — networking code typically lower than business logic, accept this).

## Open questions (T3-specific, all to be resolved at T3 entry)

- **Q-T3-1 (D1):** Path A vs Path B vs hybrid. *Default: A.*
- **Q-T3-2:** Where does the mock MQTT broker live — same Python process as pytest, or a subprocess? *Default: same process via `aiomqtt` test helpers; subprocess if isolation issues.*
- **Q-T3-3:** TLS — do we simulate `WiFiClientSecure`? *Default: yes, with a test-cert fixture and a no-verify mode that's documented as test-only.*
- **Q-T3-4:** WiFi-AP-mode (`WiFi.softAP`) — in v1 of T3 or deferred? *Default: deferred to T3.1; STA mode is the dominant case.*
- **Q-T3-5:** SmartConfig / WiFiManager-style provisioning — in T3 or T4? *Default: T4, since BLE provisioning is the modern path and that's a T4 concern.*
- **Q-T3-6:** Per-fixture vs global mock servers — does each test get its own `pytest-httpserver` instance, or is it shared? *Default: per-test fixture for isolation; document the perf trade-off.*

## What this tier explicitly will not catch (yet)

- Real-world packet loss, jitter, latency variability.
- DNS resolution edge cases (we control DNS in the sim).
- Real RF interference / scan accuracy.
- TLS handshake timing.
- IPv6 (deferred — most ESP32 IoT projects are IPv4-only).
