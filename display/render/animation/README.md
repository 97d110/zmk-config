# Display Animation Controller

`display/render/animation/` owns the generic, theme-independent animation
controller for the local dual nice!view renderer.

The controller consumes `display/core/` Display Plans and maintains renderer-
local Animation State such as frame ticks, typing phase, decay, idle-loop
progress, and visual display-sleep. These values are intentionally not part of
`display/core/`; Core State remains semantic and stable. The controller is
theme-independent: it exposes an animation snapshot that any theme consumes.

The controller is configured by a theme-supplied Timing Profile. Firmware CMake
and the host simulator convert the active theme's `timing_profile.json` (e.g.
`themes/space/v1/timing_profile.json`) into `dual_display_animation_timing.h`, so
the simulator and firmware use the same timing constants. The generated header
belongs in build output, not tracked source.

Firmware and simulator builds both compile this layer so animation behavior can
be exercised on the host before testing display-engine debug artifacts on
hardware.
