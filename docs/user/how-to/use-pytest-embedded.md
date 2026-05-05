# How to write scenario tests with pytest-embedded

In addition to the in-process Unity surface (where you assert against
`Sim::*` from C++), `esp32-pio-emulator` ships a `pytest-pio-emulator`
plugin that drives your sketch from outside via subprocess + stdout. Same
shape as Espressif's `pytest-embedded`.

## When to use which

| Test layer | Surface | Example assertion |
|---|---|---|
| Pure logic | Unity | "the parser handles malformed input" |
| Single peripheral interaction | Unity | "after `setup()`, GPIO 2 is HIGH within 500 ms" |
| Multi-step scenario | pytest-embedded | "boots → reads sensor → logs → goes idle → wakes on timer" |

If you can express the test as a Unity assertion in C++, prefer Unity (faster,
simpler). Switch to pytest-embedded when the test reads more like a
narrative against the device's *behavior over time as observed externally*.

## Install

```bash
pip install -e harness/pytest_pio_emulator
```

Or from a consumer project, install the published package (when available on PyPI).

## Pattern

```python
# tests/test_scenario.py
def test_boots_then_logs_sensor(dut):
    dut.expect("READY")
    m = dut.expect_re(r"sensor=(\d+)")
    assert int(m.group(1)) > 0
    dut.expect("DONE", timeout=20)
```

Run:

```bash
pytest tests/
```

The plugin spawns `pio test -e native` in your project root by default.
Override with `--pio-command` if you need a specific test filter.

## What the alpha doesn't yet do

Per [ADR-0004](../../decisions/0004-pytest-plugin-control-channel.md) (T2.5),
the alpha does NOT support:

- Driving virtual time from Python (`dut.sim.advance_time_ms(...)`).
- Attaching fake peripherals from Python.
- Sending input to the sketch (writing to its stdin).

For now, control the sketch from inside C++ test code; use pytest-embedded
for scenario assertions over the resulting Serial output.
