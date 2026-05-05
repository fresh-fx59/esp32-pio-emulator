# How to test a BLE-provisioning sketch

Your sketch advertises BLE services for first-time WiFi credential
provisioning, then connects to WiFi normally on subsequent boots. Tests
verify the BLE-advertise path triggers when no credentials are stored, and
the WiFi-connect path triggers when they are.

## Pattern

```cpp
#include <BLEDevice.h>
#include <Preferences.h>
#include <esp32sim_unity/esp32sim.h>

void test_first_boot_advertises_ble(void) {
    esp32sim::Sim::reset();  // NVS empty
    esp32sim::Sim::runSetup();

    auto& ble = esp32sim::Ble::instance();
    TEST_ASSERT_TRUE(ble.initialized());
    TEST_ASSERT_TRUE(ble.advertising());
    TEST_ASSERT_FALSE(ble.service_uuids().empty());
}

void test_second_boot_skips_ble_uses_wifi(void) {
    esp32sim::Sim::reset();
    Preferences p;
    p.begin("config");
    p.putString("ssid", "Home");
    p.putString("pass", "secret");
    p.end();

    esp32sim::Sim::runSetup();
    TEST_ASSERT_FALSE(esp32sim::Ble::instance().advertising());
    // ... assert WiFi.status() == WL_CONNECTED, etc.
}
```

## Caveat (T4 alpha)

The BLE fake is **stub-level** per ADR D7: `BLEDevice::init`, services, and
characteristics are recorded as state, but there's no GATT peer simulation.
Tests that need to drive a peer connecting to your characteristic and
writing to it are not possible in T4 alpha — that's deferred to a future
T4.5 with a Python-side BLE peer fixture.

What tests *can* verify:
- The right device name was set.
- The right service UUID(s) were registered.
- Advertising was started/stopped.
- The right characteristic value was set (sketch-side; no peer reads).
