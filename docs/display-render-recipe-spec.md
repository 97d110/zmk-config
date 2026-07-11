# Display Render Recipe Spec

This document defines the shared composition boundary for asset-backed display
animation. The generic command model and compositor live under
`display/render/recipe/`; the theme-specific planner that emits recipes lives
under `themes/<name>/<version>/` (first theme: `themes/space/v1/`). Increment 8A
specified this boundary; increments 8C/8D implement it.

## Boundary

`display/render/recipe/` is the home for the generic recipe command model and
1-bit compositor. The theme-specific planner (under `themes/`) sits after
`display/render/animation/`, emits recipes, and the compositor consumes them
before medium-specific renderers:

```text
Core State
  -> Display Plan
  -> Animation State
  -> Render Recipe
  -> firmware LVGL renderer or simulator canvas renderer
```

The recipe layer is separate from `display/core/` so core state remains
theme-independent. It is also separate from `display/render/lvgl/` and `sim/`
so firmware and simulator can consume the same composition decisions.

## Inputs And Outputs

Planner input:

- the existing `zmk_dual_display_animation_snapshot`,
- the animation-region bounds from the existing Display Plan,
- the theme's asset-ID vocabulary (opaque integer IDs) under
  `themes/<name>/<version>/`.

Planner output:

- one ordered recipe per display side,
- each recipe contains only deterministic composition commands,
- each command references stable asset IDs, integer coordinates, frame indexes,
  blend behavior, clipping behavior, and optional anchor/origin data.

The recipe must not contain file paths, PNG objects, LVGL objects, canvas
objects, heap-owned image buffers, or renderer-specific handles.

## Command Model

The first recipe API should support these command kinds:

- `clear_region`: fill the animation region black or white.
- `draw_points`: draw repeated point sprites from a deterministic point list.
- `draw_sprite`: draw a static or animated asset frame at a top-left position.
- `draw_sprite_masked`: draw an asset frame through its matching mask.
- `apply_clearance_mask`: clear pixels under an actor before drawing it.
- `draw_clipped_sprite`: draw a sprite that may extend beyond the animation
  region, clipping to bounds.

The first blend modes should stay small:

- `copy_with_mask` for masked sprites,
- `or_white` for stars, streaks, and glow pixels,
- `clear_black` for actor clearance masks,
- `invert_region` reserved for later glitch/error effects.

Renderers may implement these commands differently, but they must preserve the
same command order and pixel result for the 68 x 146 animation region.

## v13 Asset Interpretation

The v13 package is the first source asset set for this recipe model:

- background points: `star_dot_1`, `star_dot_2`, `star_plus_small`,
- clipped galaxy body and perimeter stars: `galaxy_core_arms`,
  `galaxy_edge_stars`,
- transient energy: `twinkle_large_glow`,
- motion overlays: `speed_streak_00` through `speed_streak_05`,
- central actor: `asteroid` with matching masks and clearance masks.

The v13 galaxy split matters for recipes: `galaxy_core_arms` is the static
galaxy body, while `galaxy_edge_stars` is an animated sparse-star layer attached
to the perimeter. Treat it as blinking distant stars, not as a broad glow ring.

`output_frames/` and GIF previews are visual reference only. They must not be
used as runtime flipbook inputs for the shared recipe path.

## Initial Central Typing Recipes

The first central-display recipe should compose the asteroid scene from the
source sprites. Exact coordinates can be tuned during implementation, but the
phase behavior should be:

- `idle`: black region, far and mid stars, clipped galaxy, asteroid actor.
- `typing-light`: idle recipe plus one or two speed streaks.
- `typing-medium`: light recipe plus more speed streaks and one twinkle.
- `typing-high`: medium recipe plus all speed streaks, two twinkles, and faster
  asteroid frame progression.
- `typing-peak`: high recipe plus denser energy effects and an asteroid
  emphasis cadence.
- `decay`: reduce speed streaks and twinkles while preserving the same shared
  Animation State timing and frame clock.

The right/peripheral recipe can remain placeholder-defined until a matching
environment asset set is available. Sleep, link-error, and fallback scenes keep
their existing override priority from Display Plan and Animation State.

## Renderer Responsibilities

Firmware renderer responsibilities:

- resolve asset IDs to packed 1-bit firmware data,
- execute recipe commands into the nice!view animation framebuffer,
- apply portrait viewport mapping through the existing LVGL boundary.

Simulator renderer responsibilities:

- resolve the same asset IDs to host-readable image data,
- execute the same recipe command sequence on canvas or test buffers,
- expose recipe and asset diagnostics without controlling Core State.

Both renderers should be able to compare output against small deterministic
recipe tests before full visual polish.
