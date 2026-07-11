# LVGL Renderer Boundary

`display/render/lvgl/` is the durable firmware renderer boundary.

The status-screen provider in this directory owns the ZMK/LVGL entry point and
screen lifecycle. It consumes `display/core/` plans and delegates drawing
through `screen_renderer.h`.

Firmware event adapters refresh the active screen through
`zmk_dual_display_status_screen_update_from_state()`. Keep that function as a
renderer-boundary entry point; event-source mapping belongs under
`display/firmware/`.

The renderer contract returns whether the renderer-local animation state wants
another frame. Firmware uses that result to schedule bounded animation refresh
work without
changing the core display state model.

Renderer code must preserve the portrait display contract from `display/core/`:
top and bottom edges are short, left and right edges are long, and the status
bar belongs on the narrow top edge.

During increment 1, the `screen_renderer.h` contract is implemented by
`display/mock/lvgl/` because the visuals are temporary placeholders. Future
increments should replace that provider with real LVGL rendering or animation
playback while keeping the screen-entry logic here.

Theme interpretation lives in `display/render/animation/` so the simulator and
firmware share the same phase and tick behavior without making that state part
of `display/core/`.

This directory should not accumulate throwaway placeholder geometry. If a shape
exists only to prove layout, put it under `display/mock/`.

The portrait-to-panel coordinate mapping belongs here, not in mock code,
because it is required by any renderer that targets the nice!view framebuffer.
