# Increment 7: Timing Profile, Simulator Tuning, And Terminology Alignment

## Summary

Increment 7 should move theme behavior from hard-coded tick scaffolding to a shared timing profile, while aligning documentation around the durable state boundaries established in increments 6-7. The simulator becomes the tuning surface, and firmware builds naturally convert the same JSON profile into C-readable constants. Visual display-sleep must not hurt battery life: after the timeout, the theme renders one frozen frame and stops requesting animation refreshes.

## Architecture Terms

- **Core State**: theme-independent facts in `display/core/dual_display_state.*`: side, role, battery/charging bucket, `idle|typing|sleep`, transport, split-link, and layer.
- **Display Plan**: render-ready, LVGL-free output from `display/core/dual_display_plan.*`; translates Core State into status-bar slots and animation-section inputs.
- **Theme State / Animation State**: theme-owned visual timeline in `display/render/theme/`; observes Display Plans but cannot mutate Core State.
- **Timing Profile**: configurable values used by Theme State for typing progression, decay, idle loop, and visual display-sleep.

## Key Changes

- Keep `display/core/` unchanged: core activity remains only `idle`, `typing`, and `sleep`.
- Add a tracked theme timing profile JSON under `display/render/theme/`, with initial values:
  - typing light starts immediately on the theme’s first observation of a Display Plan where `activity == typing`,
  - medium at `5000ms` from that observed typing start,
  - high at `12000ms`,
  - peak at `18000ms`,
  - decay arms after `1000ms` without core typing being observed,
  - high/peak -> medium at next loop boundary,
  - medium -> light after `5000ms`,
  - light -> idle after `7000ms` total decay time,
  - visual display-sleep after `30000ms` idle.
- Add a Python generator that validates the JSON and emits a C header with constants.
- Wire the generator into CMake so firmware builds generate the header automatically in the build directory and include it from `dual_display_theme.c`.
- Wire the same generator into the host simulator build so simulator and firmware consume the same timing source.
- Keep generated headers out of tracked source unless the repo later adopts generated display assets.

## Simulator Behavior

- Add a timing editor to the browser simulator for the JSON profile values.
- Let edits apply live to the host animation engine without becoming a second behavior source.
- The simulator must tune behavior from Display Plan/Core State observations, not raw keypresses directly.
- Provide export in the exact JSON shape accepted by the generator.
- Keep the browser simulator hardware-oriented for state input; the editor only tunes timing policy.
- Add scripted simulator checks for default timing transitions and JSON validation.

## Battery And Sleep

- Idle animation may loop while visually awake.
- After `display_sleep_ms`, Theme State switches to visual sleep, renders one static frame, and sets `wants_next_frame=false`.
- Visual display-sleep does not trigger ZMK global sleep or mutate Core State.
- ZMK global sleep remains a hard Core State override if firmware reports `activity == sleep`.

## Documentation

- Update `docs/display-firmware-animation-flow.md` to consistently use:
  - Core State,
  - Display Plan,
  - Theme State / Animation State,
  - Timing Profile.
- Update the wiring diagram to show:
  - `ZMK runtime -> Core State -> Display Plan -> Theme State -> renderer`.
- Clarify that `struct zmk_dual_display_animation_plan` is part of the Display Plan, not theme-owned Animation State.
- Carry forward increment 6’s boundary: firmware and simulator both compile `display/render/theme/`, and simulator canvas renders C-derived snapshots rather than controlling theme logic.
- Add `.agentic/context/display-engine-increment-7.md`.
- Update `.agentic/README.md`, `.agentic/context/repo-map.md`, `display/render/theme/README.md`, and simulator docs as needed.

## Validation

- Extend `make verify` to validate the timing JSON and run the generator.
- Add or document a fast simulator timing test target if it stays dependency-light.
- Validate:
  - JSON schema/ranges,
  - generated C header shape,
  - default phase progression,
  - loop-boundary decay behavior,
  - idle visual sleep stops refresh requests,
  - ZMK/global sleep remains an override.
- Do not run or plan local `west update` or `west build`; firmware validation remains GitHub Actions.

## Assumptions

- The timing JSON is the single source of truth for default firmware and simulator timing.
- Final values will be tuned in the simulator and committed by replacing the JSON profile.
- Real assets and storyboards come next; this increment prepares the timing system without polishing mock visuals further.
