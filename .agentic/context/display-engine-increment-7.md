# Display Engine Increment 7 Handoff

Increment 7 replaces hard-coded theme tick scaffolding with a shared Timing
Profile and aligns the docs around the durable state terms used by the display
engine.

## What Changed

- Added `display/render/theme/timing_profile.json` as the tracked default
  Timing Profile.
- Added `scripts/agentic/generate_theme_timing.py` to validate the JSON and
  generate `dual_display_theme_timing.h` into build output.
- Updated firmware CMake to generate the timing header as part of the normal
  display-engine configure path.
- Updated the host simulator build path to generate and include the same
  timing header.
- Reworked Theme State to use elapsed milliseconds instead of raw render tick
  counts.
- Added visual display-sleep: idle animation can loop while awake, then the
  theme renders one sleep frame and stops requesting refreshes after the
  configured idle timeout.
- Added a browser timing editor that updates the host C engine's Timing Profile
  live and exports the same JSON shape used by firmware defaults.
- Added `make sim-test` for scripted host timing checks.
- Updated docs to consistently use Core State, Display Plan, Theme State /
  Animation State, and Timing Profile.

## Boundary

Core State remains owned by `display/core/dual_display_state.*` and still
contains only theme-independent facts: side, role, battery/charging bucket,
semantic activity, transport, split-link, and layer.

Display Plan remains owned by `display/core/dual_display_plan.*`. It converts
Core State into status-bar render inputs and theme-independent animation-region
inputs.

Theme State / Animation State remains behind `display/render/theme/`. It may
observe Display Plans and maintain typing intensity, decay, frame timing,
idle-loop timing, and visual display-sleep, but it must not mutate Core State.

The simulator timing editor changes only the Timing Profile used by the host C
theme engine. It is not a manual Core State controller.

## Simulation

Use:

```bash
make sim
```

Open `http://localhost:8080`. The timing editor updates the host engine live
and the export pane emits JSON compatible with
`display/render/theme/timing_profile.json`.

Run scripted timing checks with:

```bash
make sim-test
```

## Firmware Debugging

Flash a `*_display_engine_debug` artifact. Complete state logs should still
show only Core State activity such as `idle`, `typing`, or `sleep`. Theme logs
show derived phase changes and elapsed timing summaries. Visual display-sleep
should stop the theme refresh loop after rendering its frozen frame.

Firmware build validation remains GitHub Actions from commits. Do not run
local `west update` or `west build` in this repo.

## Validation

Run:

```bash
python3 -m py_compile scripts/agentic/generate_theme_timing.py sim/web/app.py sim/engine/test_timing.py
python3 sim/engine/test_timing.py
make verify
git diff --check
```

## Intentionally Incomplete

- No real assets or storyboard implementation yet.
- No compositor or asset manifest yet.
- Timing values are initial defaults meant to be tuned in the simulator before
  asset-driven behavior polish.
- Cross-half synchronization remains a later timing/synchronization increment.
