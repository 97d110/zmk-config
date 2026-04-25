# LVGL Renderer Boundary

`display/render/lvgl/` is the durable firmware renderer boundary.

The status-screen provider in this directory owns the ZMK/LVGL entry point and
screen lifecycle. It consumes `display/core/` plans and delegates the actual
current drawing implementation.

Renderer code must preserve the portrait display contract from `display/core/`:
top and bottom edges are short, left and right edges are long, and the status
bar belongs on the narrow top edge.

During increment 1, drawing is delegated to `display/mock/lvgl/` because the
visuals are temporary placeholders. Future increments should replace that mock
renderer with real LVGL rendering or animation playback while keeping the
screen-entry logic here.

This directory should not accumulate throwaway placeholder geometry. If a shape
exists only to prove layout, put it under `display/mock/`.
