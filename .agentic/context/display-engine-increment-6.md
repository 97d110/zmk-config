# Display Engine Increment 6 Handoff

Increment 6 introduces the first durable theme boundary and makes the mock
animation region visibly reflect renderer-local theme state.

## What Changed

- Added `display/render/theme/`, an LVGL-free theme layer shared by firmware and
  host simulator builds.
- Added a theme context and snapshot that derive frame ticks, typing phase,
  short idle decay, scene, layer, energy, and charging interpretation from the
  existing Display Plan.
- Updated the LVGL renderer contract to return whether the theme wants another
  frame and the current theme snapshot.
- Updated the mock LVGL renderer so the status bar remains functional while the
  animation region shows central/peripheral theme differences, typing phase,
  decay, layer modifiers, energy, and charging.
- Added a firmware theme refresh work item that redraws from the latest stored
  firmware state while the theme reports active animation work.
- Keydown events now move display activity to semantic `typing` immediately;
  the existing quiet-period work still owns the return to `idle`.
- Replaced the old terminal/manual simulator path with a browser canvas
  simulator backed by a local Python serial bridge and a host C runner under
  `sim/engine/`.
- The host C runner compiles `display/core/` and `display/render/theme/` and
  emits JSON snapshots consumed by the canvas renderer.
- The simulator controller is now hardware-oriented: core keyboard logs feed
  the host C engine; firmware display/theme logs remain visible diagnostics but
  do not control canvas state.
- The simulator tracks core key events, active layers from
  `set_layer_state: layer_changed`, and USB-powered charging inference from
  visible serial sources.

## Boundary

Core state remains semantic. `display/core/` still owns only durable display
state and screen planning. It does not expose typing phases, decay counters,
frame ticks, intensity, or asset identifiers.

Theme state belongs behind `display/render/theme/`. The mock renderer consumes
that state, but all throwaway drawing geometry stays under `display/mock/`.

Simulator canvas code is a renderer only. It must not own typing lifecycle,
layer selection, charging policy, scene selection, or theme phase progression.
Those decisions come from the host C engine, which uses the same durable
`display/core/` and `display/render/theme/` code as firmware-facing paths.

Firmware display/theme logs such as `complete display state` and
`theme context changed` are diagnostic comparison data in the simulator. They
must not become simulator controllers.

## Simulation

Use:

```bash
make sim
```

Open `http://localhost:8080`. The Python server scans `/dev/serial/by-id/*`
and `/dev/ttyACM*`, normalizes core keyboard events, feeds them into
`sim/engine/dual_display_engine.c`, and returns C-derived snapshots through
`/api/serial`.

The web UI shows:

- left and right canvas displays rendered from the host C snapshot;
- the snapshot and serial parse counters;
- raw firmware logs for comparison and debugging.

There are intentionally no Web Serial controls and no manual state manager in
the browser. Restart `make sim` after changing C engine or theme code so the
host runner is rebuilt.

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
python3 -m py_compile sim/web/app.py
make verify
git diff --check
```

Use `make sim` to inspect the browser canvas simulator. A useful smoke check is
that a key event drives the host engine through `idle -> typing-light -> decay`
and that layer logs such as `layer_changed: layer 1 state 1` map to the symbol
layer. Firmware build validation remains GitHub Actions from commits. Never
plan, suggest, or attempt local `west update` or `west build`.

## Intentionally Incomplete

- No real assets or packed bitmap registry.
- No final animation timing policy.
- No cross-half synchronization clock.
- Typing phase and decay thresholds are simple proof-of-boundary behavior for
  the mock, firmware, and host C simulator; later increments can tune them
  behind the same theme API.
- Core keyboard-log parsing in the simulator is pragmatic and should grow only
  as needed for animation iteration. It should keep display/theme log parsing
  diagnostic-only.
