# Increment 8B Plan: Structural Refactor — Generic Engine vs. Theme Content

## Context

8A defined the render-recipe boundary as spec only. Before writing the first real
theme code, 8B separates **generic reusable engine modules** (kept under
`display/`) from **theme-specific content** (moved to a new root `themes/` tree).
It also renames the generic animation controller, which was misleadingly named
`theme`, to `animation`. 8B is a **behavior-preserving refactor**: no runtime
behavior changes; `make sim-test` and `verify.sh` pass identically before and
after.

This is the first of three increments (8B refactor → 8C recipe command model +
planner → 8D compositor + mock assets + rendering).

## Rename: theme → animation (generic controller)

| Kind | Before | After |
|---|---|---|
| Dir | `display/render/theme/` | `display/render/animation/` |
| Source | `dual_display_theme.{c,h}` | `dual_display_animation.{c,h}` |
| Generated header | `dual_display_theme_timing.h` | `dual_display_animation_timing.h` |
| Generator | `scripts/agentic/generate_theme_timing.py` | `generate_animation_timing.py` |
| Types/functions | `zmk_dual_display_theme_*` | `zmk_dual_display_animation_*` |
| Enum values | `ZMK_DUAL_DISPLAY_THEME_PHASE_*` | `ZMK_DUAL_DISPLAY_ANIMATION_PHASE_*` |
| Timing macros | `ZMK_DUAL_DISPLAY_THEME_*_MS` | `ZMK_DUAL_DISPLAY_ANIMATION_*_MS` |
| Config | `CONFIG_ZMK_DUAL_DISPLAY_THEME_REFRESH_PERIOD_MS` | `…_ANIMATION_REFRESH_PERIOD_MS` |
| Firmware work | `theme_refresh_work*` | `animation_refresh_work*` |

The animation controller shares the `ZMK_DUAL_DISPLAY_ANIMATION_` prefix with the
existing core geometry macros (`ZMK_DUAL_DISPLAY_ANIMATION_Y/_HEIGHT`) and
`zmk_dual_display_animation_plan` — different suffixes, no exact-name collision.
The sim JSON wire key `"theme"` and local variable names were intentionally left
as-is (deferred until the sim data shape is restructured).

## Moves (root themes/)

- `display/assets/niceview_asteroid_agent_package_v13` → `themes/space/v1/assets/…`
- `display/render/animation/timing_profile.json` → `themes/space/v1/timing_profile.json`
  (it is the space theme's tuning; the generic controller reads whichever theme's
  profile the build selects)
- `display/assets/` removed

## Consumers updated

`CMakeLists.txt`, `Kconfig`, `display/firmware/dual_display_state_adapter.{c,h}`,
`display/mock/lvgl/placeholder_renderer.c`, `display/render/lvgl/screen_renderer.h`,
`display/render/lvgl/dual_display_status_screen.c`, `sim/engine/dual_display_engine.c`,
`sim/engine/test_timing.py`, `sim/web/app.py`, `scripts/agentic/verify.sh`. Docs:
code-org convention, repo-map, `display/README.md`, renamed `animation/README.md`,
`display/render/lvgl/README.md`, `display/mock/README.md`, `sim/README.md`,
`docs/display-render-recipe-spec.md`, `docs/display-firmware-animation-flow.md`,
new `themes/README.md`.

The historical increment handoffs (0–8A) are left untouched; `verify.sh` keeps its
one historical reference to `display/render/theme/` in increment-6's handoff.

## Validation

`make sim-test` (compiles the engine from the renamed sources + generator, timing
transitions identical) and `verify.sh` both pass. Firmware build validation stays
on GitHub Actions.

Host note: this machine has no system `rg` and Docker is down, so `make verify`
and `make verify-docker` cannot run directly; `verify.sh` is run with the
Claude-provided ripgrep exposed on PATH, which is equivalent.

## Intentionally incomplete

- No recipe command model or planner yet (8C).
- No compositor, asset backend, or recipe rendering yet (8D).
- The `display/mock/` placeholder renderer remains until 8D retires it.
