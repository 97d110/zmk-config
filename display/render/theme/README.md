# Display Theme Boundary

`display/render/theme/` owns LVGL-free theme interpretation for the local dual
nice!view renderer.

The theme layer consumes `display/core/` screen plans and maintains
renderer-local animation state such as frame ticks, typing phase, and short
idle decay. These values are intentionally not part of `display/core/`; core
state remains semantic and stable.

Firmware and simulator builds both compile this layer so theme behavior can be
exercised on the host before testing display-engine debug artifacts on
hardware.
