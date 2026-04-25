# LVGL Renderer Boundary

`display/render/lvgl/` is the durable firmware renderer boundary.

The status-screen provider in this directory owns the ZMK/LVGL entry point and
screen lifecycle. It consumes `display/core/` plans and delegates drawing
through `screen_renderer.h`.

Renderer code must preserve the portrait display contract from `display/core/`:
top and bottom edges are short, left and right edges are long, and the status
bar belongs on the narrow top edge.

During increment 1, the `screen_renderer.h` contract is implemented by
`display/mock/lvgl/` because the visuals are temporary placeholders. Future
increments should replace that provider with real LVGL rendering or animation
playback while keeping the screen-entry logic here.

This directory should not accumulate throwaway placeholder geometry. If a shape
exists only to prove layout, put it under `display/mock/`.

The portrait-to-panel coordinate mapping belongs here, not in mock code,
because it is required by any renderer that targets the nice!view framebuffer.
