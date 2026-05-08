# Display Engine Increment 6 Handoff

Increment 6 introduces the first durable theme boundary and makes the mock
animation region visibly reflect renderer-local theme state.

## What Changed

- Added `display/render/theme/`, an LVGL-free theme layer shared by firmware and
  simulator builds.
- Added a theme context and snapshot that derive frame ticks, typing phase,
  short idle decay, scene, layer, energy, and charging interpretation from the
  existing core screen plan.
- Updated the LVGL renderer contract to return whether the theme wants another
  frame and the current theme snapshot.
- Updated the mock LVGL renderer so the status bar remains functional while the
  animation region shows central/peripheral theme differences, typing phase,
  decay, layer modifiers, energy, and charging.
- Added a firmware theme refresh work item that redraws from the latest stored
  firmware state while the theme reports active animation work.
- Keydown events now move display activity to semantic `typing` immediately;
  the existing quiet-period work still owns the return to `idle`.
- Updated the simulator to compile the shared theme layer and support `tick`
  commands so theme-only changes can be previewed without core state changes.

## Boundary

Core state remains semantic. `display/core/` still owns only durable display
state and screen planning. It does not expose typing phases, decay counters,
frame ticks, intensity, or asset identifiers.

Theme state belongs behind `display/render/theme/`. The mock renderer consumes
that state, but all throwaway drawing geometry stays under `display/mock/`.

## Simulation

Use:

```bash
make sim
```

Useful commands:

```text
activity left typing
tick 4
activity left idle
tick 4
sleep right on
split left disconnected
layer right 3
battery left 80 charging
```

`tick <count>` advances both theme contexts. `tick left <count>` or
`tick right <count>` advances one side.

## Firmware Debugging

Flash the `*_display_engine_debug` artifacts. Firmware logs still use the single
`zmk_dual_display` module. Expect logs for scene entry, theme context changes,
and refresh-loop start/stop. Per-frame redraws should not emit unconditional
logs.

The status bar remains event-driven by battery, split, transport, and layer
state. The animation region can redraw from the theme refresh loop even when
the core display state has not changed.

## Validation

Run:

```bash
make verify
make sim-build
git diff --check
```

Firmware build validation remains GitHub Actions from commits. Do not run local
`west build` unless explicitly requested.

## Intentionally Incomplete

- No real assets or packed bitmap registry.
- No final animation timing policy.
- No cross-half synchronization clock.
- Typing phase and decay thresholds are simple proof-of-boundary behavior for
  the mock and simulator; later increments can tune them behind the same theme
  API.
