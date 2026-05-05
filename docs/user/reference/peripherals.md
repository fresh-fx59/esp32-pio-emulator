# Peripheral fakes reference

These fakes ship with `esp32-pio-emulator`. Each implements `I2CDevice` (or
`SpiDevice`) and is included by `<peripherals/Fake<NAME>.h>`.

## DS3231 (real-time clock)

- **Real chip:** Maxim DS3231 (precision I2C RTC + 32 KB EEPROM in some variants).
- **Header:** `<peripherals/FakeDS3231.h>`
- **Default address:** 0x68.
- **Register coverage:** 0x00..0x12 (time, date, alarm 1+2, control, status, aging, temperature).
- **Test API:**
  - `setTime(hour, min, sec)`
  - `setDate(day, month, year)` — year is full e.g. 2026
  - `setTemperature(double celsius)`
  - `triggerAlarm1()` — sets A1F flag
- **Library compatibility:** RTClib::DS3231, Sodaq_DS3231, similar
  register-bank-style libraries.
- **Known gaps:** square-wave output (SQW pin), oscillator stop flag (OSF)
  not modeled.

## BMP280 (pressure + temperature)

- **Real chip:** Bosch BMP280.
- **Header:** `<peripherals/FakeBMP280.h>`
- **Default address:** 0x76 (or 0x77 — set by ADDR pin in real hardware).
- **Register coverage:** chip ID (0xD0), calibration (0x88..0x9F), ctrl_meas
  (0xF4), data registers (0xF7..0xFC).
- **Test API:**
  - `setTemperature(double celsius)`
  - `setPressure(double pascals)`
- **Library compatibility:** Adafruit_BMP280, SparkFun_BMP280. Library reads
  back the synthetic raw values; sketches see plausible (within ~0.5°C / ~50Pa) values.
- **Known gaps:** humidity (BMP280 doesn't have it; BME280 does — different fake).

## MCP23017 (16-bit I/O expander)

- **Real chip:** Microchip MCP23017.
- **Header:** `<peripherals/FakeMCP23017.h>`
- **Default address:** 0x20 (configurable 0x20..0x27 by A0/A1/A2 pins).
- **Register coverage:** IODIRA/B, GPIOA/B, OLATA/B, GPPUA/B, IPOLA/B, plus
  interrupt-related (0x04..0x11) — most read/write but interrupt firing is
  T2.5+.
- **Test API:**
  - `setPin(0..15, level)` — drive a pin's input value
  - `getPin(0..15)` — read current state
  - `setMode(pin, input=true/false)`
- **Library compatibility:** Adafruit_MCP23017, similar.
- **Known gaps:** interrupt-on-change (INT pin), BANK=1 register layout,
  sequential operation (SEQOP) toggle.

## How to add a new peripheral

See [the dev how-to](../../dev/how-to/add-a-new-peripheral-fake.md).
