#include <esp_sleep.h>
#include <esp32sim/sleep.h>

namespace {
uint64_t pending_timer_us = 0;
}

extern "C" {

void esp_sleep_enable_timer_wakeup(uint64_t time_us) {
    pending_timer_us = time_us;
}

void esp_sleep_enable_ext0_wakeup(int /*gpio_num*/, int /*level*/) {
    // T4 alpha: recorded but not modeled.
}

void esp_deep_sleep_start(void) {
    esp32sim::Sleep::instance().record_deep_sleep(pending_timer_us);
    pending_timer_us = 0;
}

esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause(void) {
    using esp32sim::WakeCause;
    auto w = esp32sim::Sleep::instance().last_wake();
    switch (w) {
        case WakeCause::EXT0:     return ESP_SLEEP_WAKEUP_EXT0;
        case WakeCause::EXT1:     return ESP_SLEEP_WAKEUP_EXT1;
        case WakeCause::TIMER:    return ESP_SLEEP_WAKEUP_TIMER;
        case WakeCause::TOUCHPAD: return ESP_SLEEP_WAKEUP_TOUCHPAD;
        case WakeCause::ULP:      return ESP_SLEEP_WAKEUP_ULP;
        case WakeCause::UNDEFINED:
        default:                  return ESP_SLEEP_WAKEUP_UNDEFINED;
    }
}

}  // extern "C"
