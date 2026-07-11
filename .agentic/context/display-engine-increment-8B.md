# Display Engine Increment 8B Handoff

Increment 8B is a behavior-preserving structural refactor that separates the
generic display engine from theme-specific content, in preparation for writing
the first real theme (8C/8D).

## What Was Implemented

- Renamed the generic animation controller `theme → animation`:
  `display/render/theme/` → `display/render/animation/`,
  `dual_display_theme.{c,h}` → `dual_display_animation.{c,h}`, all
  `zmk_dual_display_theme_*` / `ZMK_DUAL_DISPLAY_THEME_*` symbols, the
  `generate_theme_timing.py` generator, the generated
  `dual_display_animation_timing.h`, the
  `CONFIG_ZMK_DUAL_DISPLAY_ANIMATION_REFRESH_PERIOD_MS` config, and the firmware
  `animation_refresh_work*` items.
- Created a root `themes/` tree for theme-specific content and moved the v13
  asteroid asset package to `themes/space/v1/assets/` and the timing profile to
  `themes/space/v1/timing_profile.json`. Removed the old `display/assets/`.
- Updated all build/host consumers (`CMakeLists.txt`, `Kconfig`, firmware
  adapter, mock renderer, LVGL boundary, sim engine, `test_timing.py`,
  `sim/web/app.py`, `verify.sh`) and rewrote the durable docs to describe the new
  boundary. Added `themes/README.md`.

## How / Why The Boundary

Two axes decide placement: theme-independence and medium-independence.
`display/` now holds only theme-independent, reusable engine modules
(`core/`, `render/animation/`, `render/recipe/`, `render/lvgl/`, `firmware/`).
Root `themes/<name>/<version>/` holds everything specific to one theme. The old
name `theme` for the generic controller collided with this split, so it became
`animation`. Theme tuning (`timing_profile.json`) and source art moved under the
theme; the generic controller reads whichever theme's profile the build selects.

## Durable vs Temporary

Durable: the rename and the new `display/` vs `themes/` structure. Temporary:
`display/mock/lvgl/placeholder_renderer.c` still implements the renderer contract
and still draws placeholder geometry; it is retired in 8D when the recipe render
path exists.

## How To Simulate

`make sim-test` builds the host engine from `display/core/`,
`display/render/animation/`, and the renamed generator, then asserts the timing
phase progression is unchanged. `make sim` serves the browser canvas as before.

## How To Debug On Firmware

No firmware behavior changed. The display-engine debug artifacts log the same
transitions; the only difference is renamed log prefixes (e.g. "animation scene
entry", "animation refresh loop") and the renamed
`CONFIG_ZMK_DUAL_DISPLAY_ANIMATION_REFRESH_PERIOD_MS`.

## Validation Run

- `make sim-test` → `sim timing: ok` (behavior identical to pre-refactor).
- `verify.sh` → `verify: ok` (run with Claude-provided ripgrep on PATH, since
  this host has no system `rg` and Docker is down; equivalent to `make verify`).
- Firmware build validation remains GitHub Actions from commits.

## Intentionally Incomplete

- No recipe command model or scene-recipe planner yet (8C).
- No compositor, asset-source interface, mock asset backend, or recipe rendering
  yet (8D).
- Real PNG→1-bit conversion + generated C registry remain roadmap Inc 9.
