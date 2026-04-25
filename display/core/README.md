# Display Core

`display/core/` is durable engine logic.

It must stay independent of LVGL, ZMK runtime events, mock assets, and final art.
The core describes what should be drawn, not how placeholder rectangles or real
assets are rendered.

Allowed here:

- geometry constants for the display contract,
- side-aware screen-plan types,
- status-bar and animation-region planning,
- generic scene variants and future state buckets,
- mapping policy that can be shared by firmware and simulator.

Not allowed here:

- `lv_obj_t` or LVGL drawing calls,
- hard-coded placeholder shape composition,
- final art names or style-specific asset names,
- mock asset IDs,
- ZMK event listener code.
