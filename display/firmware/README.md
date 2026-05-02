# Firmware State Adapter

`display/firmware/` owns the durable bridge from ZMK runtime events into the
LVGL-free display state model in `display/core/`.

The adapter keeps side-specific firmware state, maps ZMK battery, activity,
keypress, layer, endpoint, USB, BLE, and split-link events into the same
buckets used by the simulator, and queues LVGL refresh work on ZMK's display
work queue.

Central-only APIs such as keymap and endpoint state must stay behind central
role guards so the right-side peripheral firmware can still build.
