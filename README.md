# Eyelash Sofle ZMK Config

This repository contains the Eyelash Sofle ZMK board module, user config, CI build matrix, and keymap drawing configuration.

## Prerequisites

Install the base ZMK/Zephyr local build toolchain first:

- [ZMK native setup](https://zmk.dev/docs/development/local-toolchain/setup/native)
- [Zephyr getting started guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html)
- [Zephyr west installation](https://docs.zephyrproject.org/latest/develop/west/install.html)

Terminal tools used by this repo:

| Tool | Used for | Install guide |
| --- | --- | --- |
| `bash` | `Makefile` recipes and validation scripts | Installed by default on Linux/macOS; install through your OS package manager if missing. |
| `git` | Cloning, branches, commits, and GitHub Actions workflow context | [Git install docs](https://git-scm.com/book/en/v2/Getting-Started-Installing-Git) |
| `make` | `make verify`, `make context`, and `make checklist` | [GNU Make manual](https://www.gnu.org/software/make/manual/) |
| `rg` / ripgrep | Fast validation and repo search in `scripts/agentic/verify.sh` | [ripgrep install docs](https://ripgrep.dev/download/) |
| `python3`, `pip`, `venv` | Zephyr/ZMK Python tooling and isolated CLI installs | [Zephyr getting started guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) |
| `pipx` | Recommended installer for Python CLI apps such as keymap-drawer | [pipx install docs](https://pipx.pypa.io/latest/installation/) |
| `west` | ZMK/Zephyr workspace update and local firmware builds | [west install docs](https://docs.zephyrproject.org/latest/develop/west/install.html) |
| `cmake`, `ninja`, `dtc`, Zephyr SDK | Local firmware compilation | [Zephyr getting started guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html) |
| `keymap` from `keymap-drawer` | Parsing and drawing keymap diagrams | [keymap-drawer install docs](https://pypi.org/project/keymap-drawer/) |
| `tio` | Interactive USB CDC ACM serial log capture from debug firmware | [tio install docs](https://github.com/tio/tio#4-installation) |
| `lsusb` / `usbutils` | USB device identification while debugging flashed halves | Install through your OS package manager; on Debian/Ubuntu use `sudo apt install usbutils`. |
| `timeout`, `cat`, `ls`, `sed` | Short non-interactive log captures and local inspection | Usually provided by GNU coreutils or the base OS userland. |

On Linux, add your user to the serial device group before using `tio` without `sudo`:

```bash
sudo usermod -a -G dialout "$USER"
newgrp dialout
```

## Common Commands

Run cheap structural validation:

```bash
make verify
```

See [.agentic/commands.md](.agentic/commands.md) for local west builds, debug builds, settings-reset builds, and runtime log commands.
