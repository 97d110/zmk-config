# Display Engine Increment 5.B Handoff

Increment 5.B documents the state boundary created by the core typing activity
refactor. The display engine now has two intended layers of state:

- core logical display state, owned by `display/core/` and
  `display/firmware/`,
- future theme animation state, owned by the renderer/theme implementation.

## Core Logical State

Core activity is deliberately semantic:

- `ZMK_DUAL_DISPLAY_ACTIVITY_IDLE`
- `ZMK_DUAL_DISPLAY_ACTIVITY_TYPING`
- `ZMK_DUAL_DISPLAY_ACTIVITY_SLEEP`

The core state answers what the keyboard is doing, not how a theme should
animate that fact. It must not expose typing elapsed time, typing speed,
animation buckets, decay steps, frame indices, asset names, or theme-specific
intensity values.

Firmware owns the transition into and out of semantic typing. Keypress events
mark the current typing check period, and `typing_activity_work` keeps activity
at `typing` while a period contains keypresses. When a configured check period
completes without a keypress, firmware returns activity to `idle` and logs
`typing-return-idle`.

The check period is controlled by
`CONFIG_ZMK_DUAL_DISPLAY_TYPING_CHECK_PERIOD_MS`. This value is a core logical
state debounce/check interval, not an animation frame rate or decay duration.

## Theme Animation State

Theme animation state should be introduced behind the renderer/theme boundary,
not in `display/core/`.

Future themes may derive their own values from core state transitions, such as:

- typing animation phase,
- visual intensity,
- hold or decay progress after typing becomes idle,
- frame selection,
- asset variant selection.

Those values are allowed to differ per theme. A quiet/minimal theme may snap
from typing to idle immediately. A more expressive theme may continue a local
settle or decay animation after core activity has already returned to `idle`.

## Boundary Rules

- Core state remains shared durable product state.
- Theme animation state remains renderer/theme-local policy.
- Scene fallback and link-error behavior remain scene-selection policy, not
  activity state.
- The mock renderer may use a fixed placeholder intensity for `typing`, but
  mock-only intensity choices must not leak into durable public APIs.
- If a future renderer needs timers, transition history, or animation clocks,
  add them as a theme/render subsystem concept after the screen-plan boundary.

## Current Implementation Notes

- `enum zmk_dual_display_activity_state` now carries only semantic activity.
- `struct zmk_dual_display_animation_plan` still carries activity, but that
  field is semantic input for renderer/theme interpretation.
- `display/mock/lvgl/placeholder_renderer.c` maps semantic `typing` to one
  fixed temporary placeholder intensity.
- The simulator exposes `activity <side> <idle|typing>` and `sleep <side>
  <on|off>` rather than typing milliseconds.

## Validation

- `make verify` passed after the core refactor.
- `git diff --check` passed after the core refactor.
- `make sim-build` passed after simulator controls were updated.

Firmware build validation is still handled by GitHub Actions from commits.
Do not run local `west build` unless explicitly requested.
