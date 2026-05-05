// include/esp_sleep.h — fake arduino-esp32 esp-sleep API (T4)
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ESP_SLEEP_WAKEUP_UNDEFINED = 0,
    ESP_SLEEP_WAKEUP_EXT0      = 2,
    ESP_SLEEP_WAKEUP_EXT1      = 3,
    ESP_SLEEP_WAKEUP_TIMER     = 4,
    ESP_SLEEP_WAKEUP_TOUCHPAD  = 5,
    ESP_SLEEP_WAKEUP_ULP       = 6,
} esp_sleep_wakeup_cause_t;

void esp_sleep_enable_timer_wakeup(uint64_t time_us);
void esp_sleep_enable_ext0_wakeup(int gpio_num, int level);
void esp_deep_sleep_start(void);
esp_sleep_wakeup_cause_t esp_sleep_get_wakeup_cause(void);

#ifdef __cplusplus
}  // extern "C"
#endif
