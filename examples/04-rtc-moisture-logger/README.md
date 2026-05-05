# 04-rtc-moisture-logger — load-bearing T2 acceptance

Reads time from a fake DS3231 RTC over I2C, reads moisture sensor on ADC pin
34, logs every 10 seconds via Serial. **This same sketch is driven by both
Unity (in-process, via `pio test`) AND pytest-embedded (out-of-process, via
`pytest`)** — the load-bearing T2 acceptance proof.

## Run via Unity

```bash
pio test -e native
```

Two Unity tests cover: log line after first loop, log line on next interval.

## Run via pytest-embedded

```bash
pytest tests/
```

(pytest spawns the same `pio test` binary and asserts on its stdout via
`dut.expect`.)
