# 01-blink — first end-to-end test against the simulator

This example demonstrates the smallest-possible TDD loop against
`esp32-pio-emulator`. The sketch (`src/main.cpp`) is a classic 1Hz
blink. The test (`test/test_blink/test_blink.cpp`) drives the sketch
through three checks: LED starts off, toggles at 500ms intervals,
20 toggles over 10 seconds of virtual time.

Run it:

    pio test -e native

This pulls `esp32-pio-emulator` from GitHub via `lib_deps`. First run
is slow (clone); subsequent runs are sub-second.
