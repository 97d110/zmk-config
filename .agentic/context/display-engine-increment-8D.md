# Display Engine Increment 8D Handoff

Increment 8D makes pixels flow from the recipe. It adds the generic 1-bit
compositor and asset-source interface, a temporary space/v1 mock asset backend,
and rewires firmware + simulator to render the composited recipe. The
`display/mock/` placeholder renderer is retired — the "mock" concept now means
mock *assets*, not a mock renderer.

## What Was Implemented

- `display/render/recipe/dual_display_asset_source.h`: the generic asset-backend
  interface (resolve opaque asset id -> 1-bit sprite pixels/mask/clearance;
  resolve point-set id -> coordinate list). 1-bit format: row-major, MSB-first,
  stride `(width + 7) / 8`.
- `display/render/recipe/dual_display_compositor.{h,c}`: the generic compositor.
  Executes a recipe into a 68x146 region buffer (`struct
  zmk_dual_display_region_buffer`, 1 = lit) via blends `copy_with_mask`,
  `or_white`, `clear_black`, clipping every draw to bounds. LVGL-free and
  deterministic.
- `themes/space/v1/mock/mock_assets.{h,c}`: TEMPORARY placeholder asset backend.
  Procedural 1-bit sprites (disc asteroid + mask + clearance, dot/plus stars,
  solid speed streaks, glow-disc twinkle, galaxy disc, blinking edge stars) and
  the real far/mid star coordinate tables from the v13 manifest.
- Firmware: `display/mock/lvgl/placeholder_renderer.c` became the durable
  `display/render/lvgl/screen_renderer.c`. It keeps the status-bar drawing and
  now renders the animation region by building the space/v1 recipe, compositing
  it (mock assets) into a region buffer, and blitting it onto the nice!view
  canvas (black background, white lit pixels) via the viewport mapping. The
  `display/mock/` tree, its Kconfig option, and its CMake gate are removed.
- Simulator: the host engine composites each side's recipe and emits it as
  `regionWidth/regionHeight/regionSetPixels` plus a base64 `region` in the JSON;
  `sim/web/app.py` draws that 1-bit region on the canvas. `test_recipe.py` now
  also asserts the composited pixels.

## How / Why The Boundary

The recipe command list is the seam: theme-specific planner (themes/) produces
it; the generic compositor (display/render/recipe/) executes it against an
asset-source the theme supplies. The compositor never learns a theme concept — it
resolves opaque IDs. The firmware renderer selects the active theme (space/v1)
and its asset source; a future theme-selection layer can abstract that choice.

## Durable vs Temporary

Durable: the compositor, the asset-source interface, and
`display/render/lvgl/screen_renderer.c`. Temporary: `themes/space/v1/mock/`
(placeholder 1-bit sprites), replaced by the generated converted registry
(roadmap Inc 9) behind the same asset-source interface — no compositor, command
model, or planner change required.

## How To Simulate

`make sim-test` runs `test_recipe.py`, which composites the recipe through the
mock backend and asserts pixel results: the region is 68x146, idle is non-empty,
the peripheral side has fewer lit pixels (no actor), typing adds lit pixels vs
idle, and both visual and global sleep composite a fully black region. `make sim`
serves the browser canvas, which now draws the composited region per side.

## How To Debug On Firmware

The display-engine debug artifacts log `recipe rendered: side=... scene=...
phase=... commands=... actor_asset=...` once per scene/phase change. The
animation region shows the composited scene (stars + galaxy + asteroid for
space/v1's mock assets); the status bar is unchanged.

## Validation Run

- `make sim-test` -> `sim timing: ok` + `sim recipe: ok` (recipe test includes
  compositor pixel assertions).
- `verify.sh` -> `verify: ok` (run with Claude-provided ripgrep on PATH; host has
  no system `rg` and Docker is down).
- Host-validated: the compositor + mock backend + recipe->pixels path
  (`test_recipe.py`). NOT host-validated (validated by GitHub Actions build,
  on-hardware flashing, and the browser): the firmware LVGL blit in
  `screen_renderer.c` and the `sim/web/app.py` canvas rendering. Expect a
  possible follow-up "bug fixes" pass after hardware/browser verification, per
  the repo's per-increment pattern.

## Intentionally Incomplete

- Real PNG->1-bit conversion + generated C registry (roadmap Inc 9). The mock
  galaxy is a solid disc and streaks are solid blocks — placeholders, not final
  art.
- Full manifest sine-motion paths, charging overlay, layer flavors (Inc 12), and
  the peripheral environment + glitch/error asset sets.
