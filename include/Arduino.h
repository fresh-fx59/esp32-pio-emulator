// include/Arduino.h
//
// Drop-in replacement for arduino-esp32's Arduino.h, used when compiling for
// PlatformIO's [env:native] against esp32-pio-emulator. Forwards every call
// into the framework's core engine.
//
// Consumers must `#include <Arduino.h>` exactly as they would on real hardware
// — that's the load-bearing promise of the framework. This header MUST sit at
// the root of include/ so PIO's include-path priority finds us first.
#pragma once

#include <stdint.h>

#ifdef __cplusplus
// Pull in HardwareSerial transparently — real arduino-esp32's Arduino.h does
// the same, so sketches expecting `Serial` to be available after
// `#include <Arduino.h>` work without extra includes. (HardwareSerial.h is
// guarded; safe to include redundantly.)
#include <HardwareSerial.h>
#endif

#ifdef __cplusplus

// Pin-level macros. Match arduino-esp32's values exactly.
constexpr int LOW = 0;
constexpr int HIGH = 1;

// Pin modes. Values match arduino-esp32's enum, which historically uses
// power-of-two flags. We use a smaller subset; the extra bits don't matter
// in the sim because we route through esp32sim::PinMode internally.
constexpr int INPUT          = 0x01;
constexpr int OUTPUT         = 0x02;
constexpr int INPUT_PULLUP   = 0x05;
constexpr int INPUT_PULLDOWN = 0x09;

// Interrupt modes (used by attachInterrupt in T1 task 8).
constexpr int DISABLED = 0x00;
constexpr int RISING   = 0x01;
constexpr int FALLING  = 0x02;
constexpr int CHANGE   = 0x03;
constexpr int ONLOW    = 0x04;
constexpr int ONHIGH   = 0x05;

extern "C" {
#endif

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int  digitalRead(uint8_t pin);

uint32_t millis(void);
uint32_t micros(void);
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);

// Cooperative yield. T1 implements as a no-op; T4 may make it a scheduler
// hint when FreeRTOS shim lands.
void yield(void);

// Interrupts (T1 task 8).
typedef void (*voidFuncPtr)(void);
void attachInterrupt(uint8_t interrupt_num, voidFuncPtr isr, int mode);
void detachInterrupt(uint8_t interrupt_num);

// ADC (T2 task 2). Default 12-bit resolution on ESP32-S3.
int  analogRead(uint8_t pin);
void analogReadResolution(uint8_t bits);
void analogSetAttenuation(int atten);
void analogSetPinAttenuation(uint8_t pin, int atten);

// PWM / LEDC (T2 task 3).
double  ledcSetup(uint8_t channel, double freq_hz, uint8_t resolution_bits);
void    ledcAttachPin(uint8_t pin, uint8_t channel);
void    ledcDetachPin(uint8_t pin);
void    ledcWrite(uint8_t channel, uint32_t duty);
uint32_t ledcRead(uint8_t channel);
uint32_t ledcReadFreq(uint8_t channel);
void    analogWrite(uint8_t pin, int val);  // 0-255, mapped onto channel 0

#ifdef __cplusplus
}  // extern "C"

// Real arduino-esp32 maps GPIO pin → interrupt number 1:1; we mirror that.
// Inline so it links without a definition in any TU.
inline uint8_t digitalPinToInterrupt(uint8_t pin) { return pin; }

// Pull in hardware-timer declarations transitively (real arduino-esp32 does
// the same via esp32-hal-timer.h).
#include <esp32_hwtimer.h>
#endif

// Sketches expect setup() and loop() to be defined by the user. We declare
// them here so the test harness can link against them. (In real arduino-esp32
// they're invoked from the runtime startup; in the sim, harness/unity/sim.cpp
// — task 7 — drives them via ESP32Sim::runSetup() / runLoop().)
#ifdef __cplusplus
extern "C" {
#endif
void setup(void);
void loop(void);
#ifdef __cplusplus
}
#endif
