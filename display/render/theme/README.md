# Display Theme Boundary

`display/render/theme/` owns LVGL-free theme interpretation for the local dual
nice!view renderer.

The theme layer consumes `display/core/` Display Plans and maintains
renderer-local Theme State / Animation State such as frame ticks, typing
phase, decay, idle-loop progress, and visual display-sleep. These values are
intentionally not part of `display/core/`; Core State remains semantic and
stable.

`timing_profile.json` is the default Timing Profile. Firmware CMake and the
host simulator both convert it into `dual_display_theme_timing.h`, so the
simulator and firmware use the same default timing constants. The generated
header belongs in build output, not tracked source.

Firmware and simulator builds both compile this layer so theme behavior can be
exercised on the host before testing display-engine debug artifacts on
hardware.
