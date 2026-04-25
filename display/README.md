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
- `display/firmware/` will own future ZMK event/state adapters.
- `display/assets/` is reserved for durable asset registries and final or
  long-lived placeholder assets after the generic engine is proven.

## Temporary Code

- `display/mock/` owns proof-of-concept drawing, mock icons, hard-coded
  placeholder geometry, and throwaway asset names.
- Temporary code may be visually crude and hard-coded, but it must still follow
  the portrait top-to-bottom display contract and logging convention. The top
  and bottom edges are short, and the left and right edges are long.
- Temporary code must not define the engine's public model. If a field or enum
  is needed by core planning, name it generically and keep it in `display/core/`.

## Deletion Rule

Before moving to real assets or animation playback, the implementation should
be able to delete `display/mock/` and replace its implementation of the LVGL
renderer contract with a real renderer. If that is not possible, mock-only logic
has leaked into durable code and should be moved back behind the mock boundary.
