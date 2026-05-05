# pytest-pio-emulator plugin architecture

Reference doc for contributors working on the Python harness. For user-facing
"how to write a scenario test," see
[../../user/how-to/use-pytest-embedded.md](../../user/how-to/use-pytest-embedded.md).

## Layout

```
harness/pytest_pio_emulator/
├── pyproject.toml                          # package metadata + pytest11 entry point
├── pytest_pio_emulator/
│   ├── __init__.py                         # exports Dut
│   └── plugin.py                           # the actual pytest plugin
└── tests/
    └── test_plugin.py                      # unit tests against a synthetic binary
```

## How pytest activates it

`pyproject.toml` declares an entry point:

```toml
[project.entry-points.pytest11]
pio_emulator = "pytest_pio_emulator.plugin"
```

When the package is installed, pytest auto-discovers `plugin.py` and exposes
its fixtures (`dut`) and CLI options (`--pio-command`, `--pio-cwd`,
`--pio-timeout`) to all test runs.

## The Dut class

`Dut` wraps a `subprocess.Popen` of `pio test -e native` (or whatever the
user configures). Stdout is line-buffered. The plugin maintains a `_buffer`
of all lines seen so callers can `expect` against past lines.

`expect()` / `expect_exact()` / `expect_re()` block until either a match is
found in the buffer / live stream, or the deadline expires.

## What's deferred to T2.5 (ADR-0004)

The alpha is stdout-only. T2.5 will add a control channel (Unix domain socket
or named pipe) so Python can:

- Advance virtual time in the C++ sim.
- Attach Python-driven fake peripherals.
- Write to the sketch's stdin (`Serial.read` from Python).
- Snapshot/restore sim state.

The C++ side will need a corresponding control loop: a separate thread (or
single-threaded poll) reading commands from the socket and dispatching to
`esp32sim::Sim::*`. Build flag `-DESP32_PIO_EMULATOR_CONTROL_CHANNEL=1`
enables it; default off so the alpha workflow is unchanged.

## Testing the plugin itself

Tests under `tests/` use a synthetic Python script as the "binary" so we can
exercise the subprocess + expect machinery without depending on PIO
specifically. See `tests/test_plugin.py`.
