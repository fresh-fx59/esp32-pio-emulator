"""Scenario test: drives the RTC+moisture logger via pytest-pio-emulator.

This is the second test surface for the same sketch — same source as the
Unity tests under test/, but driven from outside via subprocess + stdout.
"""
import re


def test_sketch_logs_ready_then_moisture(dut):
    dut.expect("READY")
    m = dut.expect_re(r"moisture=(\d+)")
    # In our scenario the test binary's main() runs the sketch via Sim::*
    # which seeds default values; we assert the format is well-formed.
    assert m.group(1).isdigit()
