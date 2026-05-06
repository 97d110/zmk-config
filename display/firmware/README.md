# Firmware State Adapter

`display/firmware/` owns the durable bridge from ZMK runtime events into the
LVGL-free display state model in `display/core/`.

The adapter keeps side-specific firmware state, maps ZMK battery, activity,
layer, endpoint, USB, BLE, and split-link events into the same buckets used by
the simulator, and queues LVGL refresh work on ZMK's display work queue.

Keypress events stay deliberately light on the hot input path. A press only
marks that the current typing period saw input and starts a one-second delayed
work cycle if one is not already running. The delayed work logs the complete
display state once per period, advances the typing activity bucket, and either
schedules the next one-second period or stops for the future decay protocol.

Central-only APIs such as keymap and endpoint state must stay behind central
role guards so the right-side peripheral firmware can still build.
