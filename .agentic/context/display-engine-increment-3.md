# Display Engine Increment 3 Handoff

Increment 3 adds durable scene planning and modifier composition for the
animation region while keeping the status-bar plan untouched.

## Implemented

- Added durable `zmk_dual_display_scene_kind` and
  `zmk_dual_display_energy_level` enums in `display/core/dual_display_plan.h`.
  Scene kinds are behavior-oriented: `NORMAL`, `SLEEP`, `LINK_ERROR`, and
  `FALLBACK`. Energy levels collapse battery buckets into `UNKNOWN`, `LOW`,
  `MEDIUM`, and `HIGH`.
- Extended `zmk_dual_display_animation_plan` with `scene`, `layer`, `energy`,
  and `charging` fields. Fields are ordered geometry → identity → modifiers:
  `bounds`, then `variant` and `scene`, then `activity`, `layer`, `energy`,
  and `charging`. The plan describes state; the renderer chooses how to
  present a charging cue.
- Added file-local `select_scene_kind`, `energy_from_battery`, and
  `charging_from_battery` helpers in `display/core/dual_display_plan.c`. They
  apply the priority order from the v3 spec: sleep activity overrides every
  other scene, a disconnected split link overrides remaining scenes, any
  out-of-range state enum collapses to `SCENE_FALLBACK`, and otherwise the
  scene is `SCENE_NORMAL` with activity, layer, and energy carried as
  modifiers.
- Reworked `build_animation_plan` to consume the full normalized state instead
  of `(side, activity)`. Side and role variant logic is unchanged: left maps
  to the primary scene variant and right to the secondary variant.
- Updated the mock LVGL renderer to dispatch the lower region by scene kind.
  Sleep fills the region, link-error draws a stipple plus `!` glyph, fallback
  draws a checker pattern, and normal preserves the existing placeholder
  geometry while modulating bar height by activity intensity and bar width by
  energy level. A small bolt overlay is drawn in a fixed corner of the
  animation region whenever `charging` is true.
- Added a per-variant `log_scene_change_once` gate in the mock renderer so the
  new scene-aware debug log fires only when the scene kind changes for that
  side, satisfying the hot-path log rule even once a frame loop arrives in
  later increments.

## Boundaries

- Durable additions live in `display/core/dual_display_plan.{h,c}`. The
  `animation_plan` struct grew from three to seven fields and was reordered so
  identity (`variant`, `scene`) sits ahead of modifiers (`activity`, `layer`,
  `energy`, `charging`); the only callers are the planner itself and the mock
  renderer, both updated in-place.
- Per-scene placeholder drawings, the bolt overlay shape, and the `!` glyph
  remain temporary mock behavior under `display/mock/lvgl/`.
- The status-bar plan path (`build_status_bar_plan` and the status slot
  renderers) is untouched. Status-bar planning stays a sibling of animation
  planning.
- Side normalization, viewport mapping, and the LVGL renderer contract in
  `display/render/lvgl/` are unchanged.
- No final-art names leak into `display/core/`. Scene kinds and energy levels
  are behavior-oriented per the code-organization convention.

## Logging

- Sleep, link-error, and normal scene selections each emit one debug line in
  `select_scene_kind`. Out-of-range state enums emit a recoverable warning and
  fall back to `SCENE_FALLBACK`.
- `energy_from_battery` logs an `energy=UNKNOWN` debug line when the battery
  bucket is unknown.
- `build_animation_plan` finishes with a debug summary that covers scene,
  variant, activity, layer, energy, charging, and bounds.
- The mock renderer logs once per scene-kind change per side, gated by a
  static last-scene tracker indexed by scene variant. No unconditional logs
  were added to the LVGL render path.

## Validation

- Run `make verify` after this increment. New `require_match` lines pin the
  enums, helpers, log strings, and mock scene dispatch.
- The dual_display_plan ABI grows by appending fields, so existing C99
  designated-initializer call sites remain valid; mock and firmware were
  rebuilt mentally against the new shape.
- Firmware builds were not run from this environment; CI on `build.yaml`
  covers the firmware-side guarantee for left, right, studio, and the
  existing `*_display_engine_debug` artifacts that already enable display
  debug logging.
