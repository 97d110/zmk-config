# Increment 8C Plan: Recipe Command Model + space/v1 Scene-Recipe Planner

## Context

With 8B's structure in place, 8C adds the first real theme code. It splits along
the axis established in discussion:

- **Generic** composition instruction set → `display/render/recipe/` (references
  assets by **opaque integer ID**, so `display/` stays theme-blind).
- **Theme-specific** program → `themes/space/v1/` (this theme's asset vocabulary
  + the planner that emits recipe commands from an animation snapshot).

The recipe is produced as **data** this increment: emitted in the sim's JSON and
asserted by host tests, and logged (summary only) in firmware. Pixels are still
drawn by the placeholder renderer; actual recipe rendering is 8D.

## Boundary

```
display/render/animation (generic)  -> animation snapshot
themes/space/v1/scene_recipe (theme-specific)  -> recipe commands (opaque asset IDs)
display/render/recipe (generic command model)  <- defines the command structs
```

Prereq: 8B (task #1–#3).

## New files

- `display/render/recipe/dual_display_recipe.h` — generic command model:
  - kinds: `clear_region`, `draw_points`, `draw_sprite`, `draw_sprite_masked`,
    `apply_clearance_mask`, `draw_clipped_sprite`;
  - blends: `copy_with_mask`, `or_white`, `clear_black`, `invert_region` (last
    reserved);
  - `struct zmk_dual_display_recipe_command { kind; blend; uint16_t asset;
    uint16_t point_set; int16_t x,y; uint8_t frame; bool clip; }`;
  - `struct zmk_dual_display_recipe { struct zmk_dual_display_rect region;
    uint8_t command_count; command commands[ZMK_DUAL_DISPLAY_RECIPE_MAX_COMMANDS]; }`
    (fixed capacity, no heap/LVGL/paths);
  - `const char *zmk_dual_display_recipe_command_kind_name(...)` for diagnostics.
- `themes/space/v1/assets.h` — this theme's asset-ID + point-set-ID constants
  mapping the v13 assets to opaque ints (`asteroid`, `star_dot_1/2`,
  `star_plus_small`, `twinkle_large_glow`, `speed_streak_00..05`,
  `galaxy_core_arms`, `galaxy_edge_stars`; point sets `far_stars`, `mid_stars`).
- `themes/space/v1/scene_recipe.{c,h}` — the planner:
  `void zmk_dual_display_space_v1_build_recipe(const struct zmk_dual_display_animation_snapshot *,
  const struct zmk_dual_display_animation_plan *, struct zmk_dual_display_recipe *out);`
- `sim/engine/test_recipe.py` — host recipe assertions (mirrors `test_timing.py`).

## Planner behavior

Base composition per the manifest `layer_order`:
`clear_region(black)` → `draw_points(far_stars, or_white)` →
`draw_clipped_sprite(galaxy_core_arms, copy_with_mask, clip, x≈-7 y≈58)` →
`draw_clipped_sprite(galaxy_edge_stars, or_white, clip, frame=tick%64)` →
`draw_points(mid_stars, or_white)` → *[phase effects]* →
`apply_clearance_mask(asteroid, clear_black, frame=rot)` →
`draw_sprite_masked(asteroid, copy_with_mask, frame=rot)`.

Phase effect deltas (`docs/display-render-recipe-spec.md`): `idle` none;
`typing-light`/`decay` 1–2 speed streaks; `typing-medium` ~3–4 streaks + 1
twinkle; `typing-high` all 6 streaks + 2 twinkles + faster asteroid frames;
`typing-peak` all 6 + 2 twinkles (denser) + asteroid emphasis.

Frame derivation from `frame_tick`: asteroid `(tick/4)%16` (smaller divisor at
high/peak); `galaxy_edge_stars` + `twinkle_large_glow` `tick%64`. Positions are
**simplified deterministic** behind swappable helpers (`asteroid_position(tick)`,
etc.); full manifest sine paths deferred. `secondary`/peripheral variant = a
placeholder subset (clear + stars + galaxy, no actor). `sleep` = clear only;
`link_error`/`fallback` = clear + a minimal deterministic marker (no glitch
assets yet).

## Wiring (recipe as data)

- `sim/engine/dual_display_engine.c` — after each side's animation snapshot, build a
  recipe and emit an **additive** `"recipe"` JSON object (count + each command's
  kind/asset/frame/blend/clip/x/y). Add sources to `test_timing.py` and
  `test_recipe.py` build lists.
- Firmware renderer — call the planner and log a **gated** `LOG_DBG` summary on
  scene/phase change (count, phase, scene, actor asset). Placeholder pixels
  unchanged. No per-frame logs.

## Validation

`make sim-test` (adds `test_recipe.py`: per-phase command structure/order,
streak/twinkle counts scale idle→peak and drop in decay, asteroid frame advances
faster at high/peak, `sleep` clear-only, `secondary` omits the actor),
`make verify` (extended guards + runs `test_recipe.py`), `make sim` manual check
that `recipe` appears in the JSON.

## Durable vs. temporary

Durable: `display/render/recipe/` command model, `themes/space/v1/{assets.h,
scene_recipe.*}`, sim recipe emission, `test_recipe.py`. Temporary: the firmware
log-summary call inside the placeholder renderer (removed in 8D).

## Intentionally incomplete

- No compositor / asset backend / real rendering (8D).
- Positions simplified; full sine choreography deferred.
- Charging overlay, layer flavors, peripheral environment + glitch assets
  deferred.
