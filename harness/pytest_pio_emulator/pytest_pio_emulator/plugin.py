"""pytest-pio-emulator plugin entry point.

Provides a `dut` fixture that spawns a PlatformIO native test binary as a
subprocess, reads its stdout line by line, and exposes `dut.expect()` for
scenario tests.

Alpha scope (T2): subprocess + stdout reading. No control socket, no
Python-side time advancement, no fixture-attached fake peripherals. Those
land in T2.5 with ADR-0004.
"""
from __future__ import annotations

import re
import shlex
import subprocess
import time
from pathlib import Path
from typing import Optional, Pattern, Union

import pytest


class DutTimeout(AssertionError):
    """Raised when ``dut.expect()`` times out before matching."""


class Dut:
    """A running PlatformIO native test binary, drivable as a subprocess.

    Typical usage in a test::

        def test_my_sketch(dut):
            dut.expect("READY")
            dut.expect_re(r"sensor=(\\d+)")

    The `dut` fixture is provided by this plugin. By default it spawns
    ``pio test -e native`` in the current working directory. Override via
    pytest config (the ``pio_emulator_command`` ini option) or per-test by
    requesting a custom factory.
    """

    def __init__(
        self,
        command: list[str],
        cwd: Optional[Path] = None,
        timeout_default: float = 10.0,
    ):
        self._command = command
        self._cwd = cwd or Path.cwd()
        self._timeout_default = timeout_default
        self._proc: Optional[subprocess.Popen] = None
        # Buffer of all stdout lines we've seen so far. expect() searches this
        # buffer in addition to live-reading new lines.
        self._buffer: list[str] = []

    # -- lifecycle ----------------------------------------------------------

    def start(self) -> None:
        """Start the subprocess. Idempotent within one Dut lifetime."""
        if self._proc is not None:
            return
        self._proc = subprocess.Popen(
            self._command,
            cwd=str(self._cwd),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,  # line-buffered
        )

    def stop(self, timeout: float = 5.0) -> int:
        """Stop the subprocess; return its exit code (terminating if needed)."""
        if self._proc is None:
            return 0
        try:
            self._proc.wait(timeout=timeout)
        except subprocess.TimeoutExpired:
            self._proc.terminate()
            try:
                self._proc.wait(timeout=2.0)
            except subprocess.TimeoutExpired:
                self._proc.kill()
                self._proc.wait()
        rc = self._proc.returncode if self._proc.returncode is not None else -1
        self._proc = None
        return rc

    # -- expect API ---------------------------------------------------------

    def expect(
        self,
        pattern: Union[str, Pattern],
        timeout: Optional[float] = None,
    ) -> str:
        """Block until ``pattern`` appears in any stdout line.

        ``pattern`` is matched as a substring (if str) or a regex (if a
        compiled Pattern). Returns the matched line. Raises ``DutTimeout``
        if no match within ``timeout`` seconds.
        """
        return self._expect_impl(pattern, timeout, regex=False)

    def expect_exact(self, line: str, timeout: Optional[float] = None) -> str:
        """Block until a stdout line exactly equals ``line`` (after strip)."""
        return self._expect_impl(line, timeout, regex=False, exact=True)

    def expect_re(
        self,
        pattern: Union[str, Pattern],
        timeout: Optional[float] = None,
    ) -> "re.Match":
        """Block until ``pattern`` (regex) matches a stdout line.

        Returns the match object so callers can extract groups.
        """
        if isinstance(pattern, str):
            pattern = re.compile(pattern)
        deadline = time.monotonic() + (timeout if timeout is not None else self._timeout_default)
        # Search the buffer first
        for line in self._buffer:
            m = pattern.search(line)
            if m:
                return m
        # Live-read until match or timeout
        while time.monotonic() < deadline:
            line = self._read_line(deadline - time.monotonic())
            if line is None:
                continue
            self._buffer.append(line)
            m = pattern.search(line)
            if m:
                return m
        raise DutTimeout(
            f"timeout waiting for regex {pattern.pattern!r} after {timeout or self._timeout_default}s; "
            f"last {min(5, len(self._buffer))} lines: {self._buffer[-5:]}"
        )

    def _expect_impl(
        self,
        pattern: Union[str, Pattern],
        timeout: Optional[float],
        regex: bool,
        exact: bool = False,
    ) -> str:
        if regex:
            return self.expect_re(pattern, timeout).string
        deadline = time.monotonic() + (timeout if timeout is not None else self._timeout_default)
        # Search the buffer first
        for line in self._buffer:
            if (exact and line.strip() == str(pattern)) or (
                not exact and str(pattern) in line
            ):
                return line
        while time.monotonic() < deadline:
            line = self._read_line(deadline - time.monotonic())
            if line is None:
                continue
            self._buffer.append(line)
            if (exact and line.strip() == str(pattern)) or (
                not exact and str(pattern) in line
            ):
                return line
        last = self._buffer[-5:] if self._buffer else []
        raise DutTimeout(
            f"timeout waiting for {pattern!r} after "
            f"{timeout or self._timeout_default}s; last {len(last)} lines: {last}"
        )

    def _read_line(self, max_wait: float) -> Optional[str]:
        """Read one line from stdout, blocking up to max_wait seconds."""
        if self._proc is None or self._proc.stdout is None:
            return None
        # subprocess.Popen.stdout is line-buffered (bufsize=1) so readline
        # returns when a newline arrives. We rely on overall expect() timeout
        # rather than fine-grained per-line wait — keeps the alpha simple.
        # If the process has exited, readline returns "".
        line = self._proc.stdout.readline()
        if line == "":
            # process exited
            return None
        return line.rstrip("\r\n")

    # -- raw access ---------------------------------------------------------

    @property
    def buffer(self) -> list[str]:
        """All stdout lines seen so far (oldest first)."""
        return list(self._buffer)

    @property
    def returncode(self) -> Optional[int]:
        if self._proc is None:
            return None
        return self._proc.poll()


# -- pytest fixtures --------------------------------------------------------


def pytest_addoption(parser):
    group = parser.getgroup("pio_emulator")
    group.addoption(
        "--pio-command",
        default="pio test -e native",
        help="Shell command to launch the PIO native test binary "
        "(default: 'pio test -e native').",
    )
    group.addoption(
        "--pio-cwd",
        default=None,
        help="Working directory for the PIO command (default: rootdir).",
    )
    group.addoption(
        "--pio-timeout",
        type=float,
        default=10.0,
        help="Default expect() timeout in seconds (default: 10.0).",
    )


@pytest.fixture
def dut(request) -> Dut:
    """Default Dut fixture: spawns pio test in the rootdir.

    Tests can also instantiate Dut directly for custom commands::

        def test_x():
            with Dut(["pio", "test", "-e", "native", "--filter", "test_foo"]) as d:
                ...
    """
    cmd_str = request.config.getoption("--pio-command")
    cwd_str = request.config.getoption("--pio-cwd")
    timeout = request.config.getoption("--pio-timeout")
    cwd = Path(cwd_str) if cwd_str else Path(request.config.rootpath)

    d = Dut(shlex.split(cmd_str), cwd=cwd, timeout_default=timeout)
    d.start()
    try:
        yield d
    finally:
        d.stop()
