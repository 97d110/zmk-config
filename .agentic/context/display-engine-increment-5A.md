# Display Engine Increment 5.A Handoff

Increment 5.A refines the firmware activity path from per-key typing-state
updates into a lightweight one-second activity cycle. The goal is to keep the
hot keypress path cheap while preserving enough timing state to drive animation
behavior and future decay logic.

## Implemented

- Replaced per-key typing streak calculation in
  `display/firmware/dual_display_state_adapter.c` with a boolean typing-period
  marker.
- Added `typing_activity_work`, a one-second delayed work item that owns typing
  animation state after keypresses occur.
- Changed keycode press handling so it only marks
  `typing_period_had_keypress = true` and starts the delayed work cycle when no
  cycle is already active.
- Suppressed keycode release logging and no-op logs for handled keycode events
  so USB debug output is not flooded during normal typing.
- Changed runtime ZMK `ACTIVE` activity events so they no longer directly drive
  typing animation buckets. Idle and sleep events still update activity state
  directly and cancel the active typing cycle.
- Added once-per-period complete state logs that include side, role, battery,
  activity, transport, split link, layer, typing seconds, and whether the last
  period saw a keypress.
- Added the `typing-decay-pending` lifecycle point. When a one-second period
  completes without a keypress, the typing cycle stops and logs this state; the
  actual decay protocol is intentionally left for the next design step.
- Added `docs/display-firmware-animation-flow.md`, a durable technical diagram
  and explanation of the firmware animation-control flow.
- Updated `display/firmware/README.md`, `.agentic/context/repo-map.md`, and
  `scripts/agentic/verify.sh` to document and verify the new lifecycle.

## Runtime Behavior

The firmware display state remains event-driven for battery, charging, USB,
endpoint, BLE, layer, split-link, idle, and sleep changes.

Typing now follows this cycle:

1. The first keypress in an inactive period sets a boolean and schedules a
   one-second delayed work item.
2. Additional keypresses during the period only keep the boolean true.
3. At the end of the second, delayed work logs the complete display state.
4. If there was a keypress, it increments `typing_activity_seconds`, maps that
   value into the existing activity bucket model, redraws only if the visual
   bucket changed, clears the boolean, and schedules the next one-second period.
5. If there was no keypress, it resets the active typing-period fields, logs
   `typing-decay-pending`, and stops scheduling until the next keypress or
   until a future decay protocol is implemented.

## Design Notes

- The keypress path still subscribes to `zmk_keycode_state_changed`, but it no
  longer performs timestamp streak math, complete state mapping, per-key logs,
  or direct display redraws.
- Complete state logs are deliberately tied to the one-second activity cycle,
  not every key event.
- Typing animation state is now controlled by elapsed one-second periods rather
  than raw key timestamps.
- The decay entry point is explicit, but decay behavior is not implemented in
  this increment.

## Validation

- `make verify` passed.
- `git diff --check` passed.
- `make sim-build` was run after the firmware adapter change and passed.

Firmware build validation is still handled by GitHub Actions from commits.
Do not run local `west build` unless explicitly requested.
