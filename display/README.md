# Display Engine Boundaries

The display engine is split into durable engine code and temporary proof-of-
concept code. Keep these boundaries strict so placeholder work can be deleted
without rewriting the core.

## Durable Code

- `display/core/` owns LVGL-free contracts: display dimensions, side identity,
  status slot plans, animation-region plans, and generic scene variants.
- `display/render/lvgl/` owns the firmware LVGL adapter boundary and the ZMK
  `zmk_display_status_screen()` entry point, renderer contract, and viewport
  mapping.
- `display/render/theme/` owns LVGL-free theme interpretation and renderer-
  local animation state that can be shared by firmware and simulator builds.
- `display/firmware/` owns ZMK event/state adapters that translate firmware
  runtime sources into the durable `display/core/` state model.
- `display/assets/` is reserved for durable asset registries and final or
  long-lived placeholder assets after the generic engine is proven.

## State Terms

- Core State is the theme-independent `zmk_dual_display_state`: side, role,
  battery/charging bucket, `idle|typing|sleep`, transport, split-link, and
  layer.
- Display Plan is the LVGL-free render input derived from Core State. It
  translates status-bar slots and carries theme-independent animation-section
  inputs.
- Theme State / Animation State belongs under `display/render/theme/`. It may
  react to Display Plans, but it must not mutate Core State.
- Timing Profile values live in `display/render/theme/timing_profile.json` and
  are generated into C-readable build output.

## Temporary Code

- `display/mock/` owns proof-of-concept drawing, mock icons, hard-coded
  placeholder geometry, and throwaway asset names.
- Temporary code may be visually crude and hard-coded, but it must still follow
  the portrait top-to-bottom display contract and logging convention. The top
  and bottom edges are short, and the left and right edges are long.
- Temporary code must not define the engine's public model. If a field or enum
  is needed by core planning, name it generically and keep it in `display/core/`.
  If a value describes theme timing, phase, or frame progression, keep it behind
  `display/render/theme/`.

## Deletion Rule

Before moving to real assets or animation playback, the implementation should
be able to delete `display/mock/` and replace its implementation of the LVGL
renderer contract with a real renderer. If that is not possible, mock-only logic
has leaked into durable code and should be moved back behind the mock boundary.
