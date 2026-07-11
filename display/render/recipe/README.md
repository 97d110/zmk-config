# Render Recipe (Generic Composition)

`display/render/recipe/` owns the generic, theme-independent composition layer:
the recipe command model (`dual_display_recipe.h`) and — added in 8D — the 1-bit
compositor and the asset-source interface.

A **recipe** is an ordered, renderer-neutral list of composition commands for the
68x146 animation region. It is produced by a theme-specific planner (under
`themes/<name>/<version>/`) and executed by a renderer/compositor. It contains
only deterministic commands referencing **opaque integer asset IDs** — never LVGL
objects, canvas objects, package-relative file paths, or heap-owned buffers.

This layer never learns what an asset ID means. A theme's `assets.h` gives the IDs
meaning and its asset backend resolves them to pixels and point coordinates, so
the same recipe renders identically on firmware and in the simulator.
