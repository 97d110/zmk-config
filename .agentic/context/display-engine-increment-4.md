# Display Engine Increment 4 Handoff

Increment 4 adds an Ubuntu console simulator for the dual-display scene engine.
It shares the durable `display/core/` state and planning code with firmware and
keeps the preview renderer local to `sim/`.

## Implemented

- Added `sim/dual_display_sim.c`, an interactive host simulator that builds
  left and right default states, accepts manual state commands, rebuilds the
  dual screen plan, and renders both screens as a compact ASCII preview.
- Added `sim/Makefile` with `make run`, `make all`, and `make clean` targets.
  The root `Makefile` now exposes `make sim`, `make sim-build`, and
  `make sim-clean`.
- Added `sim/web/app.py` and `sim/web/Dockerfile` so the simulator can run as a
  browser UI locally or inside Docker. The web app replays browser state
  through `sim/build/dual_display_sim --batch`.
- Updated `display/log.h` so non-Zephyr host builds emit
  `zmk_dual_display` logs to stderr instead of dropping them. Zephyr builds
  still use the existing `LOG_MODULE_DECLARE` path.
- Documented simulator usage in `sim/README.md` and `.agentic/commands.md`.
- Ignored `sim/build/` as a local generated host-build output directory.

## Boundaries

- Durable planning stays in `display/core/`. The simulator compiles
  `display/core/dual_display_state.c` and `display/core/dual_display_plan.c`
  directly rather than duplicating planner behavior.
- ASCII preview logic is simulator-only and lives under `sim/`; it does not
  define public engine fields, scene names, or renderer contracts.
- Web UI state is adapter state only. Planning and scene selection still happen
  in the compiled C simulator, which uses the durable core planner.
- Firmware build wiring, `build.yaml`, `config/west.yml`, and the LVGL render
  path are unchanged.

## Manual Scenarios

The simulator supports these state controls:

- `battery <left|right> <percent> [charging]`
- `activity <left|right> <typing_ms>`
- `sleep <left|right> <on|off>`
- `split <left|right> <unknown|connected|disconnected>`
- `transport <left|right> <unknown|usb|bt|disconnected>`
- `layer <left|right> <0-3>`
- `show`, `help`, and `quit`

During validation, a scripted session exercised low battery, charging high
battery, USB transport, right-side link-error, left-side sleep override, and
right-side layer mode changes.

## Validation

- `make sim-build` passed.
- `make sim-web` was smoke-tested locally.
- `./sim/build/dual_display_sim` ran interactively and showed adjacent
  `zmk_dual_display` core logs while state changes rebuilt the dual plan.
- `make verify` passed after the simulator checks were added.
