# How to test a WiFi-connecting sketch

Your sketch calls `WiFi.begin(ssid, password)` and you want to test that it
connects, reports the right state, and behaves correctly when WiFi is
disconnected.

## Pattern

```cpp
#include <WiFi.h>
#include <esp32sim_unity/esp32sim.h>

void test_wifi_connects(void) {
    esp32sim::Sim::reset();
    esp32sim::Sim::runSetup();
    TEST_ASSERT_EQUAL_INT(WL_CONNECTED, WiFi.status());
    TEST_ASSERT_EQUAL_STRING("MyNetwork", WiFi.SSID());
}
```

`WiFi.begin()` immediately transitions to `WL_CONNECTED` in the sim — no
real connection. To test reconnect logic, drive transitions explicitly:

```cpp
esp32sim::Network::instance().disconnect_wifi();
// run loop iterations; sketch should call WiFi.begin again
```

## What this catches / doesn't catch

**Catches:** sketch logic that depends on `WiFi.status()`, `WiFi.SSID()`,
`WiFi.localIP()`, `WiFi.RSSI()`. Reconnection state machines.

**Doesn't catch:** real RF behavior (range, signal loss), DHCP timing,
WPA2-Enterprise edge cases, AP-mode interactions. T3.5+ if needed.
