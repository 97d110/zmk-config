# Firmware State Adapter

`display/firmware/` owns the durable bridge from ZMK runtime events into the
LVGL-free display state model in `display/core/`.

The adapter keeps side-specific firmware state, maps ZMK battery, semantic
activity, layer, endpoint, USB, BLE, and split-link events into the same state
model used by the simulator, and queues LVGL refresh work on ZMK's display work
queue.

Keypress events stay deliberately light on the hot input path. A press only
sets semantic display activity to `typing`, marks that the current typing check
period saw input, and starts a delayed work cycle if one is not already running.
The delayed work logs the complete display state once per period, keeps semantic
activity at `typing` while keypresses are observed, and returns activity to
`idle` after a quiet period.

Renderer-local theme refreshes also run from this layer. They redraw from the
latest stored firmware state only while the renderer reports active theme
frames, such as typing phase or short idle decay.

Central-only APIs such as keymap and endpoint state must stay behind central
role guards so the right-side peripheral firmware can still build. Layer state
is mirrored to the peripheral display through the local `DDL_SYNC` split
behavior instead of asking the right half to link central-only keymap APIs.
