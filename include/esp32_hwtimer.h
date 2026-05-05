// include/esp32_hwtimer.h — fake arduino-esp32 hardware timer API
//
// Real arduino-esp32 declares these in <esp32-hal-timer.h>, transitively
// through Arduino.h. We expose them via <Arduino.h> too (extern "C") plus
// this dedicated header for direct inclusion if a sketch wants it.
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle. The real arduino-esp32 declares hw_timer_t as a typedef
// to a private struct; we mirror that.
typedef struct hw_timer_handle hw_timer_t;

hw_timer_t* timerBegin(uint8_t num, uint16_t divider, bool count_up);
void        timerEnd(hw_timer_t* timer);
void        timerAttachInterrupt(hw_timer_t* timer, void (*fn)(void), bool edge);
void        timerDetachInterrupt(hw_timer_t* timer);
void        timerAlarmWrite(hw_timer_t* timer, uint64_t alarm_value, bool autoreload);
void        timerAlarmEnable(hw_timer_t* timer);
void        timerAlarmDisable(hw_timer_t* timer);
uint64_t    timerRead(hw_timer_t* timer);

#ifdef __cplusplus
}  // extern "C"
#endif
