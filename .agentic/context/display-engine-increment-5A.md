# Display Engine Increment 5.A Handoff

Increment 5.A refines the firmware activity path from per-key typing-state
updates into a lightweight semantic typing check cycle. The goal is to keep the
hot keypress path cheap while exposing only core activity states: idle, typing,
and sleep.

## Implemented

- Replaced per-key typing streak calculation in
  `display/firmware/dual_display_state_adapter.c` with a boolean typing-period
  marker.
- Added `typing_activity_work`, a configurable delayed work item that owns
  semantic typing state after keypresses occur.
- Changed keycode press handling so it only marks
  `typing_period_had_keypress = true` and starts the delayed work cycle when no
  cycle is already active.
- Suppressed keycode release logging and no-op logs for handled keycode events
  so USB debug output is not flooded during normal typing.
- Changed runtime ZMK `ACTIVE` activity events so they no longer directly drive
  animation-specific typing values. Idle and sleep events still update activity
  state directly and cancel the active typing cycle.
- Added once-per-period complete state logs that include side, role, battery,
  activity, transport, split link, layer, the configured period, and whether
  the last period saw a keypress.
- When a configured period completes without a keypress, the typing cycle sets
  semantic activity back to idle, logs `typing-return-idle`, and stops.
- Added `docs/display-firmware-animation-flow.md`, a durable technical diagram
  and explanation of the firmware animation-control flow.
- Updated `display/firmware/README.md`, `.agentic/context/repo-map.md`, and
  `scripts/agentic/verify.sh` to document and verify the new lifecycle.

## Runtime Behavior

The firmware display state remains event-driven for battery, charging, USB,
endpoint, BLE, layer, split-link, idle, and sleep changes.

Typing now follows this cycle:

1. The first keypress in an inactive period sets a boolean and schedules a
   delayed work item using `CONFIG_ZMK_DUAL_DISPLAY_TYPING_CHECK_PERIOD_MS`.
2. Additional keypresses during the period only keep the boolean true.
3. At the end of the second, delayed work logs the complete display state.
4. If there was a keypress, it keeps semantic activity at `typing`, redraws
   only if the visual state changed, clears the boolean, and schedules the next
   check period.
5. If there was no keypress, it resets the active typing-period fields, sets
   semantic activity to `idle`, logs `typing-return-idle`, and stops scheduling
   until the next keypress.

## Design Notes

- The keypress path still subscribes to `zmk_keycode_state_changed`, but it no
  longer performs timestamp streak math, complete state mapping, per-key logs,
  or direct display redraws.
- Complete state logs are deliberately tied to the configurable activity check
  cycle, not every key event.
- Theme-specific typing intensity, decay, and animation timing are intentionally
  outside the core activity state.

## Validation

- `make verify` passed.
- `git diff --check` passed.
- `make sim-build` was run after the firmware adapter change and passed.

Firmware build validation is still handled by GitHub Actions from commits.
Do not run local `west build` unless explicitly requested.
