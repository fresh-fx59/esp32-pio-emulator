"""Tests for the pytest-pio-emulator plugin.

These run the plugin's Dut against a tiny synthetic Python script (not a real
PIO binary) to validate the subprocess + expect machinery.
"""
import sys
import time
from pathlib import Path

import pytest

from pytest_pio_emulator import Dut
from pytest_pio_emulator.plugin import DutTimeout


@pytest.fixture
def synth_dut(tmp_path):
    """A Dut wrapping a tiny Python script that prints lines with delays."""
    script = tmp_path / "fake_binary.py"
    script.write_text(
        "import sys, time\n"
        "for line in ['READY', 'sensor=42', 'sensor=43', 'DONE']:\n"
        "    print(line, flush=True)\n"
        "    time.sleep(0.05)\n"
    )
    d = Dut([sys.executable, str(script)], cwd=tmp_path, timeout_default=3.0)
    d.start()
    try:
        yield d
    finally:
        d.stop()


def test_expect_substring(synth_dut):
    line = synth_dut.expect("READY")
    assert "READY" in line


def test_expect_subsequent(synth_dut):
    synth_dut.expect("READY")
    line = synth_dut.expect("DONE")
    assert "DONE" in line


def test_expect_re_with_groups(synth_dut):
    synth_dut.expect("READY")
    m = synth_dut.expect_re(r"sensor=(\d+)")
    assert m.group(1) == "42"


def test_expect_timeout_raises(synth_dut):
    synth_dut.expect("DONE")  # consume up to end
    with pytest.raises(DutTimeout):
        synth_dut.expect("WONT_APPEAR", timeout=0.5)


def test_buffer_accumulates(synth_dut):
    synth_dut.expect("DONE")
    assert "READY" in synth_dut.buffer[0]
    assert any("sensor=42" in line for line in synth_dut.buffer)


def test_dut_stop_returns_exit_code(synth_dut):
    synth_dut.expect("DONE")
    rc = synth_dut.stop()
    assert rc == 0
