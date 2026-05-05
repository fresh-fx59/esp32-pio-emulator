# pytest-pio-emulator

Pytest plugin that drives an `esp32-pio-emulator`-based test binary as a
subprocess. Provides a `dut` fixture exposing `expect()` / `expect_re()` for
scenario tests — same shape as Espressif's `pytest-embedded` and the wider
embedded-testing ecosystem.

## Install (alpha)

```bash
cd harness/pytest_pio_emulator
pip install -e .
```

## Usage

In a project that uses `esp32-pio-emulator` as its native test target:

```python
# tests/test_scenario.py
def test_device_publishes_sensor(dut):
    dut.expect("READY")
    m = dut.expect_re(r"sensor=(\d+)")
    assert int(m.group(1)) > 0
    dut.expect("DONE")
```

Run:

```bash
pytest --pio-command "pio test -e native --filter test_logger_scenario"
```

## Alpha scope

This is the T2 alpha. The minimal viable surface:

- ✅ Spawn a subprocess (default: `pio test -e native`).
- ✅ Read its stdout line by line.
- ✅ `dut.expect(substring)`, `dut.expect_exact(line)`, `dut.expect_re(regex)`.
- ✅ Buffer of all seen lines.
- ❌ Python-side time advancement (deferred to T2.5 / ADR-0004).
- ❌ Python-side fake peripheral attachment (deferred).
- ❌ Bidirectional control (writing to sketch's stdin) (deferred).

## Architecture

See [`docs/dev/reference/pytest-plugin-architecture.md`](../../docs/dev/reference/pytest-plugin-architecture.md)
for the full design (T2 task 12).

## License

MIT.
