# Display Engine Increment 8C Handoff

Increment 8C adds the first real theme code: a generic recipe command model plus
the space theme's scene-recipe planner. The recipe is produced as data — asserted
in host tests and logged on firmware — but not yet rendered to pixels (that is
8D).

## What Was Implemented

- `display/render/recipe/dual_display_recipe.{h,c}`: the generic,
  theme-independent recipe command model. Command kinds (`clear_region`,
  `draw_points`, `draw_sprite`, `draw_sprite_masked`, `apply_clearance_mask`,
  `draw_clipped_sprite`), blend modes (`copy_with_mask`, `or_white`,
  `clear_black`, `invert_region` reserved), a fixed-capacity
  `struct zmk_dual_display_recipe`, and init/push/name helpers. Assets are
  referenced by opaque `uint16_t` IDs.
- `themes/space/v1/assets.h`: the space theme's asset-ID + point-set-ID
  vocabulary (opaque ints for asteroid, stars, twinkle, speed streaks, galaxy
  layers; far/mid star point sets).
- `themes/space/v1/scene_recipe.{h,c}`:
  `zmk_dual_display_space_v1_build_recipe(animation_snapshot, animation_plan,
  out)`. Composes the asteroid scene in the manifest layer order; scales speed
  streaks and twinkles by typing phase; derives animated frames from
  `frame_tick`; uses simplified deterministic positions behind swappable helpers.
  The peripheral (secondary) variant is a placeholder environment view (no
  actor); non-normal scenes and visual display-sleep emit a frozen clear-only
  recipe.
- Wiring: `sim/engine/dual_display_engine.c` emits an additive `"recipe"` object
  per side in its JSON snapshot; the firmware placeholder renderer builds the
  recipe and logs a gated `recipe built: …` summary on scene/phase change (its
  placeholder pixels are unchanged). `sim/engine/test_recipe.py` asserts the
  per-phase command structure. Both are wired into `make sim-test` and
  `verify.sh`. CMake compiles `dual_display_recipe.c` and
  `themes/space/v1/scene_recipe.c`.

## How / Why The Boundary

The recipe command list is the seam between theme-specific policy and generic
execution. The command model is generic (references opaque IDs, knows no theme),
so it lives under `display/render/recipe/`. The planner is theme-specific (knows
the asteroid vocabulary and composition), so it lives under `themes/space/v1/`.
This keeps `display/` theme-blind and lets a future theme ship a different planner
against the same command model.

## Durable vs Temporary

Durable: the recipe command model, the space/v1 asset vocabulary and planner, the
sim recipe emission, and `test_recipe.py`. Temporary: the `log_recipe_summary_once`
call inside the mock placeholder renderer (it disappears with `display/mock/` in
8D). Positions are simplified-deterministic placeholders behind helpers.

## How To Simulate

`make sim-test` runs `test_timing.py` and `test_recipe.py`. The recipe test drives
the host engine and asserts: idle draws the base scene with no effects and the
actor present; typing-light/medium/high/peak scale `draw_sprite` counts to
2/5/8/8; animated frames advance with ticks; decay reduces effects; visual sleep
and global sleep both collapse to a clear-only recipe; the peripheral side omits
the actor. `make sim` streams the `recipe` object in each JSON snapshot.

## How To Debug On Firmware

The display-engine debug artifacts log `recipe built: side=… scene=… phase=…
commands=… actor_asset=…` once per scene/phase change. No per-frame logging.

## Validation Run

- `make sim-test` -> `sim timing: ok` + `sim recipe: ok`.
- `verify.sh` -> `verify: ok` (run with Claude-provided ripgrep on PATH; this host
  lacks a system `rg` and Docker is down).
- Firmware build validation remains GitHub Actions from commits.

## Intentionally Incomplete

- No compositor, asset-source interface, mock asset backend, or recipe rendering
  yet (8D); recipes are data only.
- Simplified positions; full manifest sine motion deferred. typing-peak currently
  mirrors typing-high's command counts (denser/emphasis cadence deferred).
- Charging overlay, layer flavors, and peripheral environment / glitch assets
  deferred. Real PNG->1-bit conversion + generated registry remain roadmap Inc 9.
