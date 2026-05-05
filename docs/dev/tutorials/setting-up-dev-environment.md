# Setting up your development environment

This tutorial gets you from a fresh clone to "I can run the framework's own tests" in
under 10 minutes.

## Prerequisites

- Linux (Ubuntu 22.04+) or macOS (13+). Windows is untested — try WSL2 if you must.
- Python 3.10+
- A C++ compiler. On Ubuntu: `sudo apt install build-essential`. On macOS: `xcode-select --install`.

## 1. Install PlatformIO (in a project-local venv)

PlatformIO is the build system for the framework's own tests as well as for users adopting
us downstream. We install it in a project-local virtualenv at `.venv/` so we get a recent
version that actually works on current Python (the system-packaged `pio` on Ubuntu
24.04+ / Python 3.12 is too old and crashes on `cli.resultcallback`).

```bash
cd esp32-pio-emulator       # if you're not already there
python3 -m venv .venv
.venv/bin/pip install --upgrade pip
.venv/bin/pip install platformio
```

Verify: `.venv/bin/pio --version` should print `PlatformIO Core, version 6.x.x`.

For convenience, activate the venv in your shell so `pio` is on your PATH:

```bash
source .venv/bin/activate
pio --version
```

The `.venv/` directory is gitignored.

## 2. Clone the repo

```bash
git clone https://github.com/fresh-fx59/esp32-pio-emulator
cd esp32-pio-emulator
```

## 3. Run the test suite

```bash
pio test -e native
```

Expected at the end of T0: at least one test passes (the skeleton smoke test). As tiers
ship, the suite grows.

## 4. (Optional) Set up pre-commit hooks

We use the [pre-commit](https://pre-commit.com/) framework to enforce style locally.

```bash
python3 -m pip install --user pre-commit
pre-commit install
```

Now `git commit` will run `clang-format`, trailing-whitespace fix, end-of-file fix, and YAML
lint before every commit. CI runs the same checks, so a missed local hook is caught before
merge.

## 5. (Optional) Editor setup

The repo ships `.editorconfig` and `.clang-format` files. Install the relevant extension for
your editor (most modern editors honor EditorConfig out of the box).

For VS Code:

- C/C++ extension (Microsoft) — picks up `.clang-format`
- EditorConfig for VS Code — picks up `.editorconfig`

## What's next

- Read [`../explanation/architecture-overview.md`](../explanation/architecture-overview.md)
  for a 10-minute architectural tour.
- Find the active tier's plan under [`../../superpowers/plans/`](../../superpowers/plans/).
- Pick a task from the active plan; ask in an issue or PR before starting if it's substantial.
