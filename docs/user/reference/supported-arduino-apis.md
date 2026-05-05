# Supported Arduino APIs (T1)

Tier 1 implements the subset of `arduino-esp32` APIs that cover GPIO + Serial + timing.
That's enough to test-drive any sketch whose hardware footprint is digital pins, the serial
console, and time. I2C, SPI, ADC, PWM, WiFi, filesystem land in later tiers.

## Pins

| API | Behavior |
|---|---|
| `pinMode(pin, mode)` | `INPUT`, `OUTPUT`, `INPUT_PULLUP`, `INPUT_PULLDOWN`. Mode change emits `GPIO_PIN_MODE` event. `INPUT_PULLUP` sets level=1 at mode-change time; external `setLevel(LOW)` overrides. |
| `digitalWrite(pin, val)` | `HIGH`/`LOW`. Emits `GPIO_WRITE` event. Fires any registered listeners (interrupts, peripherals). |
| `digitalRead(pin)` | Returns the pin's current level. |

## Time

| API | Behavior |
|---|---|
| `millis()` | Virtual milliseconds since `Sim::reset()`. |
| `micros()` | Virtual microseconds since `Sim::reset()`. |
| `delay(ms)` | Advances virtual clock by `ms`. **Does not sleep.** |
| `delayMicroseconds(us)` | Advances virtual clock by `us`. |
| `yield()` | No-op in T1. T4 may make it a scheduler hint. |

## Serial / UART

`Serial`, `Serial1`, `Serial2` are global `HardwareSerial` instances. The `begin(baud)`
parameter is ignored (the sim has no electrical representation).

| API | Behavior |
|---|---|
| `Serial.begin(baud)` | No-op. |
| `Serial.print(...)` | Multiple overloads: `const char*`, `int`, `long`, `unsigned`, `double`, `char`. `print(int, base)` supports bases 2/8/10/16. |
| `Serial.println(...)` | Same as `print` plus `\r\n`. |
| `Serial.printf(fmt, ...)` | Up to 256 chars (truncated beyond). |
| `Serial.write(byte)`, `write(buf, n)`, `write(const char*)` | Raw byte output. |
| `Serial.read()` | Consumes one byte from RX, returns `-1` if empty. |
| `Serial.peek()` | Returns next RX byte without consuming, `-1` if empty. |
| `Serial.available()` | Number of bytes in RX. |
| `Serial.flush()` | No-op. |

## Interrupts

| API | Behavior |
|---|---|
| `attachInterrupt(num, isr, mode)` | `RISING`/`FALLING`/`CHANGE`/`ONHIGH`/`ONLOW`. ISR fires *synchronously* when the pin's level changes in the matching direction. **No real preemption** — see [what this does and doesn't catch](../explanation/what-this-does-and-doesnt-catch.md). |
| `detachInterrupt(num)` | Silences the ISR; future level changes do nothing. |
| `digitalPinToInterrupt(pin)` | Identity (pin number = interrupt number on ESP32). |

## Not yet supported in T1

I2C (`Wire`), SPI, ADC, PWM/LEDC, hardware timers — Tier 2.
WiFi, HTTPS, MQTT, NTP, mDNS — Tier 3.
Filesystem (LittleFS/SPIFFS), NVS, deep-sleep, RMT, BLE — Tier 4.

If your sketch uses any of these, those calls won't link. T1 is GPIO + Serial only.
