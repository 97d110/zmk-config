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
- `display/render/animation/` owns the generic, theme-independent animation
  controller: typing phase, decay, frame clock, and the animation snapshot
  shared by firmware and simulator builds.
- `display/render/recipe/` owns the generic, theme-independent composition
  instruction set and 1-bit compositor. It references assets by opaque integer
  ID; the theme-specific planner that emits recipe commands lives under
  `themes/<name>/<version>/`.
- `display/firmware/` owns ZMK event/state adapters that translate firmware
  runtime sources into the durable `display/core/` state model.

Theme-specific content (scene-recipe planner, asset-ID vocabulary, timing
profile, source assets, mock asset backend) lives under root
`themes/<name>/<version>/` — e.g. `themes/space/v1/` — never under `display/`.

## State Terms

- Core State is the theme-independent `zmk_dual_display_state`: side, role,
  battery/charging bucket, `idle|typing|sleep`, transport, split-link, and
  layer.
- Display Plan is the LVGL-free render input derived from Core State. It
  translates status-bar slots and carries theme-independent animation-section
  inputs.
- Animation State belongs under `display/render/animation/`. It may
  react to Display Plans, but it must not mutate Core State.
- Render Recipe belongs under `display/render/recipe/`. It references assets by
  opaque integer ID and carries composition commands, but never LVGL objects,
  browser objects, package-relative file paths, or Core State fields. The planner
  that produces it is theme-specific and lives under `themes/`.
- Timing Profile values live in `themes/space/v1/timing_profile.json` and
  are generated into C-readable build output.

## Temporary Code

- The only temporary code is a theme's mock asset backend
  (`themes/<name>/<version>/mock/`): hand-authored placeholder 1-bit sprites that
  stand in until the real converted asset registry exists (roadmap Inc 9).
- It must not define the engine's public model. If a field or enum is needed by
  core planning, name it generically and keep it in `display/core/`. If a value
  describes animation timing, phase, or frame progression, keep it in
  `display/render/animation/`.

## Deletion Rule

The mock asset backend is deletable behind the `display/render/recipe/`
asset-source interface: swapping in the generated registry must not change the
compositor, the recipe command model, or any theme planner. If a change would,
mock-only logic has leaked into durable code and should move back behind the
asset-source boundary.
