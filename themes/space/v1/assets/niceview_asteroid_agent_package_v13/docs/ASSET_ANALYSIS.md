# v13 Asset Analysis

This package is source sprite material for the shared render-recipe planner.
`output_frames/` and `example/` are reference renders only. They show the
package's assembled animation target, but firmware and simulator should not
consume them as runtime flipbook input.

## Display Contract

- Animation region: 68 x 146 px.
- Full nice!view portrait frame: 68 x 160 px.
- Status bar reserve: 14 px at the narrow top edge.
- Runtime composition should render into the animation region, then each
  renderer maps that region into its own display medium.

## Asset Roles

| Asset | Role | Size | Frames | Masks | Notes |
|---|---:|---:|---:|---:|---|
| `star_dot_1` | background point | 1 x 1 | 1 | 1 | Reusable far or mid star particle. |
| `star_dot_2` | background point | 2 x 2 | 1 | 1 | Brighter reusable star particle. |
| `star_plus_small` | background point | 3 x 3 | 1 | 1 | Accent reusable star particle. |
| `galaxy_core_arms` | clipped galaxy body | 129 x 98 | 1 | 1 | Static sheet, larger than the viewport. Uses `origin_in_sprite=[55,48]`. |
| `galaxy_edge_stars` | clipped galaxy perimeter stars | 129 x 98 | 64 | 64 | Animated sparse distant stars attached to the galaxy edge. Uses `origin_in_sprite=[55,48]`. |
| `twinkle_large_glow` | transient energy sprite | 13 x 13 | 64 | 64 | Center anchored with `anchor=[6,6]`. |
| `speed_streak_00` | motion streak | 11 x 12 | 1 | 1 | Typing-intensity overlay candidate. |
| `speed_streak_01` | motion streak | 9 x 10 | 1 | 1 | Typing-intensity overlay candidate. |
| `speed_streak_02` | motion streak | 8 x 9 | 1 | 1 | Typing-intensity overlay candidate. |
| `speed_streak_03` | motion streak | 9 x 10 | 1 | 1 | Typing-intensity overlay candidate. |
| `speed_streak_04` | motion streak | 8 x 9 | 1 | 1 | Typing-intensity overlay candidate. |
| `speed_streak_05` | motion streak | 6 x 7 | 1 | 1 | Smallest typing-intensity overlay candidate. |
| `asteroid` | central actor | 32 x 32 | 16 | 16 | Has matching `clearance_masks/` for erasing a black border before drawing the actor. |

## Galaxy Change From v12

The galaxy composition is the v13-specific change:

- `galaxy_core_arms` now contains the core, arms, inner ring, and non-sparse
  portions of the former outer ring.
- `galaxy_edge_stars` replaces `galaxy_outer_glow` as the animated galaxy
  companion layer.
- `galaxy_edge_stars` should be interpreted as sparse distant perimeter stars
  that blink over time, not as a broad glow or ring.
- Both galaxy assets use the same origin and should be drawn at the same
  clipped top-left position.

## Composition Hints

The package manifest's reference render order is:

```text
black_canvas
far_stars
galaxy_core_arms
galaxy_edge_stars
mid_stars
twinkle_large_glow_instances
speed_streak_instances
asteroid_clearance_mask
asteroid_sprite
```

Use this order as a reference for the first shared recipe, but do not make it a
hard-coded rendering law. Later typing-intensity recipes may choose different
subsets, positions, frame offsets, and densities.

Useful source formulas from the manifest and assembly guide:

- Global frame count: 64 frames.
- Reference frame duration: 70 ms.
- Asteroid actor frame: `floor(frame_index / 4) % 16`.
- Original asteroid motion: starts around `(11,20)`, moves toward `(17,46)`,
  and uses sine wobble.
- Galaxy origin: centered around screen coordinate `(48,106)` with a small
  one-pixel sine drift. Because the galaxy sheets are larger than 68 x 146,
  renderers must clip them to the animation region.
- Twinkle instances travel from outside the visible region into or across it,
  using center anchoring.
- Parallax layer offsets in the manifest are reference values. The shared
  recipe planner may derive simpler deterministic offsets from Theme State
  frame timing.

## Validation Snapshot

Read-only validation performed for this analysis:

- 324 asset PNGs are referenced by `manifest.json`.
- All 324 referenced asset PNGs exist.
- No unreferenced PNGs exist under `assets/`.
- All referenced PNG dimensions match their declared asset sizes.
- All referenced PNGs are PIL mode `1`.
- All referenced PNGs contain only black and white pixels.
- `output_frames/` contains 64 reference frames at 68 x 146 px.

## Runtime Boundary

The shared recipe planner should refer to assets by stable asset IDs from an
asset registry. It must not refer to package-relative file paths, browser image
objects, LVGL objects, or simulator-only constructs. Firmware and simulator
renderers are responsible for resolving those IDs to their local asset backend.
