# Display Engine Increment 2 Handoff

Increment 2 adds the shared state model and makes status slots value-aware.

## Implemented

- Added durable display state and mapping helpers under `display/core/`.
- Moved display side ownership into the state contract so firmware, simulator,
  and planning share one side model.
- Added battery buckets for `0-10`, `11-50`, and `51-100` percent, with
  matching charging variants.
- Added activity buckets for idle, sleep, and typing streaks capped at 2s, 5s,
  10s, and 15s.
- Added transport, split-link, and layer-mode state enums.
- Added default side-derived firmware state:
  - left is central,
  - right is peripheral,
  - layer is `Type`,
  - activity is idle,
  - battery, transport, and split-link are unknown.
- Added state-aware screen-plan builders while keeping existing side-only
  wrappers for compatibility.
- Expanded status slot plans from active/inactive to typed values so renderers
  can distinguish unknown, disconnected, charging, transport, split, and layer
  states.
- Carried the activity bucket into the animation plan so future animation
  selection can use the shared state contract without another plan-shape change.
- Updated the mock LVGL renderer to draw placeholder battery fill/charging,
  USB/BT/disconnected transport, split-link, layer letters, and slash overlays
  for unknown values.

## Boundaries

- State mapping and status-value planning are durable core behavior.
- Placeholder icon geometry and glyph drawings remain temporary mock behavior.
- Firmware continues to include the durable renderer contract and viewport
  mapper only; it does not include mock renderer headers.
- `display/render/lvgl/viewport.c` remains the only portrait-to-framebuffer
  coordinate mapper.

## Logging

- State mapping helpers log decisions at debug level.
- Invalid layer indexes and battery percentages log recoverable warnings and
  map to unknown values.
- The firmware status screen logs initial default state capture through the
  state-transition helper.
- The mock renderer logs slash-overlay rendering for unknown status values.

## Validation

- Run `make verify` after this increment.
- Firmware builds were not required for this increment, but if `west` is
  available, left and right `nice_view` builds are useful follow-up checks.
