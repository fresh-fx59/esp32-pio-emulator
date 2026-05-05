# 06-mqtt-temperature — T3 networking acceptance

Sketch connects to WiFi, fetches its MQTT topic from an HTTP config endpoint,
then publishes temperature readings to that topic over MQTT every 5 seconds.

This is the load-bearing T3 acceptance: the sketch hits all three new
networking surfaces (WiFi state, HTTP request, MQTT publish), and tests
verify each interaction via Sim::Network event recording — without ever
touching a real network.

```bash
pio test -e native
```
