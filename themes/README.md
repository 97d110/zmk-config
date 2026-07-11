# Themes

Root home for theme-specific display content. Everything under `display/` is the
generic, theme-independent engine; everything here is what a single theme
provides. A theme is swappable and versioned:

```
themes/<name>/<version>/
```

The first theme is `themes/space/v1/` (asteroid space scene).

## What a theme owns

- `scene_recipe.{c,h}` — the planner: turns an animation snapshot
  (`display/render/animation/`) plus the animation-region bounds into an ordered
  list of generic recipe commands (`display/render/recipe/`). *(added in 8C)*
- `assets.h` — this theme's asset-ID and point-set-ID vocabulary. The generic
  recipe layer references assets only by **opaque integer ID**; this header
  gives those IDs meaning. *(added in 8C)*
- `timing_profile.json` — this theme's Timing Profile tuning. Firmware CMake and
  the host simulator generate C constants from it for
  `display/render/animation/`.
- `assets/` — source art (1-bit PNG packages + manifests).
- `mock/` — a temporary placeholder asset backend so the recipe can render with
  stand-in sprites before real converted assets exist. Deletable. *(added in 8D)*

## What a theme must NOT contain

- Generic engine code (state, planning, the animation controller, the recipe
  command model, the compositor, LVGL glue). Those live under `display/`.
- Anything another theme would need to reuse — promote it to the generic layer
  instead.

## Contract

A theme is consumed through one generic seam: its planner emits recipe commands
that reference opaque asset IDs, and its asset backend resolves those IDs to
1-bit pixels. Swapping the theme (or adding `space/v2`) must not require changes
under `display/`.
