# Increment 7: Timing Profile, Simulator Tuning, And Build-Generated C Config

## Summary

Increment 7 should move theme behavior from hard-coded tick scaffolding to a shared timing profile. The simulator becomes the place to tune values, and firmware builds naturally convert the same JSON profile into C-readable constants. Visual display-sleep must not hurt battery life: after the timeout, the theme renders one frozen frame and stops requesting animation refreshes.

## Key Changes

- Keep `display/core/` unchanged: core activity remains `idle`, `typing`, and `sleep`.
- Add a tracked theme timing profile JSON under `display/render/theme/`, with initial values:
  - typing light starts immediately on first keypress,
  - medium at `5000ms`,
  - high at `12000ms`,
  - peak at `18000ms`,
  - decay arms after `1000ms` without keypresses,
  - high/peak -> medium at next loop boundary,
  - medium -> light after `5000ms`,
  - light -> idle after `7000ms` total decay time,
  - visual display-sleep after `30000ms` idle.
- Add a Python generator that validates the JSON and emits a C header with constants.
- Wire the generator into CMake so firmware builds generate the header automatically in the build directory and include it from `dual_display_theme.c`.
- Wire the same generator into the host simulator build so simulator and firmware consume the same timing source.
- Keep generated headers out of git-tracked source unless the repo already requires generated artifacts.

## Simulator Behavior

- Add a timing editor to the browser simulator for the JSON profile values.
- Let edits apply live to the host animation engine without becoming a second behavior source.
- Provide export in the exact JSON shape accepted by the generator.
- Keep the browser simulator hardware-oriented for state input; the editor only tunes timing policy.
- Add scripted simulator checks for default timing transitions and JSON validation.

## Battery Constraint

- Idle animation may loop while the display is visually awake.
- After `display_sleep_ms`, the theme switches to a visual sleep phase, renders one static frame, and sets `wants_next_frame=false`.
- ZMK global sleep remains a hard override if firmware reports `ZMK_ACTIVITY_SLEEP`.
- Visual display-sleep must not trigger ZMK power-state changes or increase refresh work after the frozen frame.

## Validation And Docs

- Extend `make verify` to validate the timing JSON and run the generator.
- Add or document a fast simulator timing test target if it stays dependency-light.
- Add `.agentic/context/display-engine-increment-7.md`.
- Update `.agentic/README.md`, `.agentic/context/repo-map.md`, and simulator docs.
- Do not run or plan local `west update` or `west build`; firmware validation remains GitHub Actions.

## Assumptions

- The timing JSON is the single source of truth for default firmware and simulator timing.
- Final values will be tuned in the simulator and then committed by replacing the JSON profile.
- Real assets and storyboards come next; this increment prepares the timing system without polishing mock visuals further.
