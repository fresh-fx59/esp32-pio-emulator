# 07-deep-sleep-mqtt — T4 reference (full chip)

The kitchen-sink reference example. Sketch demonstrates:

- **NVS / Preferences** for persistent config + boot counter.
- **BLE** for first-time provisioning (when no WiFi credentials in NVS).
- **WiFi** to connect when credentials are present.
- **MQTT** to publish a telemetry message on each wake.
- **Deep sleep** for 60 seconds, then re-runs setup() on wake.

Three tests prove the lifecycle:
1. First boot (no NVS creds) starts BLE advertising.
2. With NVS creds, sketch connects WiFi, publishes MQTT, sleeps 60 s.
3. NVS-stored boot count persists across two `runSetup()` invocations.

```bash
pio test -e native
```
