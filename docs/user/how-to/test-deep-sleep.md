# How to test a deep-sleep cycle

Your sketch enters deep-sleep with `esp_deep_sleep_start()` and re-runs
`setup()` on wake. Tests need to verify the sleep duration, the wake reason,
and that NVS-persisted state survives the cycle.

## Pattern

```cpp
#include <Preferences.h>
#include <esp_sleep.h>
#include <esp32sim_unity/esp32sim.h>

void test_publishes_then_sleeps_60s(void) {
    esp32sim::Sim::reset();

    // Pre-seed NVS so sketch has WiFi credentials, etc.
    Preferences p;
    p.begin("config");
    p.putString("ssid", "Home");
    p.end();

    esp32sim::Sim::runSetup();

    // Verify deep sleep was requested for 60 s.
    TEST_ASSERT_EQUAL_INT(1, esp32sim::Sleep::instance().deep_sleep_count());
    TEST_ASSERT_EQUAL_UINT64(60000000ULL, esp32sim::Sleep::instance().last_sleep_us());
}

void test_multiple_wake_cycles_increment_boot_count(void) {
    esp32sim::Sim::reset();
    Preferences p; p.begin("config"); p.putString("ssid", "x"); p.end();

    esp32sim::Sim::runSetup();  // boot 1
    esp32sim::Sim::runSetup();  // boot 2 (simulated wake)
    esp32sim::Sim::runSetup();  // boot 3

    Preferences q; q.begin("config");
    TEST_ASSERT_EQUAL_UINT32(3, q.getUInt("boots"));
}
```

## Wake reason

Pre-set the wake cause your sketch should observe:

```cpp
esp32sim::Sleep::instance().set_next_wake_cause(esp32sim::WakeCause::TIMER);
esp_deep_sleep_start();
TEST_ASSERT_EQUAL_INT(ESP_SLEEP_WAKEUP_TIMER,
                      (int)esp_sleep_get_wakeup_cause());
```

## What this catches / doesn't

**Catches:** sketch logic that depends on wake reason, sleep-duration math,
NVS-persisted state, boot-counter style patterns.

**Doesn't:** real RTC drift across deep-sleep, wake-time energy consumption,
brownout-during-wake edge cases. These remain real-hardware-only concerns.
