# Display Engine Increment 4 Handoff

Historical note: this increment originally added a host-side simulator. That
implementation has since been removed. Current simulator work uses the browser
canvas app under `sim/web/`, a local Python serial bridge, and the host C
engine under `sim/engine/`.

## Implemented

- Added the first simulator workflow for host-side display inspection. This
  workflow has been superseded by the canvas simulator with a local serial
  bridge and host C engine.
- Added `sim/web/app.py` and `sim/web/Dockerfile` so the simulator can run as a
  browser UI locally or inside Docker.
- Updated `display/log.h` so non-Zephyr host builds emit
  `zmk_dual_display` logs to stderr instead of dropping them. Zephyr builds
  still use the existing `LOG_MODULE_DECLARE` path.
- Documented simulator usage in `sim/README.md` and `.agentic/commands.md`.
- Ignored `sim/build/` as a local generated host-build output directory.

## Boundaries

- Durable planning stays in `display/core/`.
- Canvas drawing stays in `sim/web/`; serial scanning stays in the local Python
  bridge; host display/theme derivation stays in `sim/engine/`.
- Web UI state is adapter state only.
- Firmware build wiring, `build.yaml`, `config/west.yml`, and the LVGL render
  path are unchanged.

## Controller Boundary

The connected keyboard's core events are the controller for normal use.
Firmware display/theme logs are diagnostic comparison data only; canvas state is
driven from snapshots emitted by the host C engine.

## Validation

- `make sim-web` was smoke-tested locally.
- `make verify` passed after the simulator checks were added.
