# Display Engine Increment 8A Handoff

Increment 8A analyzes the v13 asteroid asset package as reusable sprite source
material and defines the shared render-recipe boundary that future firmware and
simulator renderers should consume.

## What Changed

- Added `display/assets/niceview_asteroid_agent_package_v13/docs/ASSET_ANALYSIS.md`.
- Added `docs/display-render-recipe-spec.md`.
- Updated display-engine context docs to include the planned
  `display/render/recipe/` boundary.
- Kept `output_frames/` and GIF previews explicitly reference-only.

## Boundary

Core State and Display Plan remain generic and theme-independent under
`display/core/`.

Theme State / Animation State remains under `display/render/theme/` and still
owns typing phases, decay, frame timing, and sleep timing.

The next shared composition layer should live under `display/render/recipe/`.
It consumes Theme State snapshots plus animation bounds and emits ordered,
renderer-neutral composition commands. It must not depend on LVGL, browser
canvas objects, package-relative file paths, or generated firmware buffers.

The v13 asset package remains source sprite material. Its rendered
`output_frames/` are useful for visual comparison and inspiration only.

## Asset Findings

The v13 package provides:

- background point sprites: `star_dot_1`, `star_dot_2`, `star_plus_small`,
- clipped galaxy body and perimeter stars: `galaxy_core_arms`,
  `galaxy_edge_stars`,
- transient twinkle sprites: `twinkle_large_glow`,
- motion streak overlays: `speed_streak_00` through `speed_streak_05`,
- the central asteroid actor with frame masks and clearance masks.

The v13-specific galaxy change is that `galaxy_core_arms` now contains the core,
arms, inner ring, and non-sparse former outer-ring pixels, while
`galaxy_edge_stars` is the animated sparse perimeter-star layer. It should not
be treated as the old broad `galaxy_outer_glow` layer.

Read-only metadata validation confirmed that all manifest-referenced asset
PNGs exist, match declared dimensions, are 1-bit black/white images, and have
no extra unreferenced PNGs under `assets/`.

## Recipe Spec

The recipe spec defines the first command set:

- clear region,
- draw point fields,
- draw static or animated sprites,
- draw masked sprites,
- apply actor clearance masks,
- draw clipped sprites.

Asset references should be stable registry IDs. Renderer-specific objects and
file paths stay behind firmware or simulator asset backends.

## Simulation

No simulator behavior changed in this increment. Future simulator work should
render the same recipe emitted by the shared planner instead of using
`output_frames/` as a flipbook.

## Firmware Debugging

No firmware behavior changed in this increment. Future firmware work should
consume the same recipe command stream as the simulator after asset conversion
and packed 1-bit registries exist.

Firmware build validation remains GitHub Actions from commits. Do not run
local `west update` or `west build` in this repo.

## Validation

Run:

```bash
python3 - <<'PY'
import json
from pathlib import Path
from PIL import Image

root = Path("display/assets/niceview_asteroid_agent_package_v13")
data = json.loads((root / "manifest.json").read_text())
refs = set()
for asset in data["assets"].values():
    expected = tuple(asset["size"])
    for frame in asset["frames"]:
        for key in ("frame", "mask", "clearance_mask"):
            if key not in frame:
                continue
            path = root / frame[key]
            refs.add(path.resolve())
            with Image.open(path) as image:
                assert image.size == expected
                assert image.mode == "1"
                assert set(image.convert("L").getdata()).issubset({0, 255})

asset_pngs = {path.resolve() for path in (root / "assets").rglob("*.png")}
assert refs == asset_pngs
print(f"validated {len(refs)} referenced asset PNGs")
PY
make verify
git diff --check
```

## Intentionally Incomplete

- No shared recipe planner implementation yet.
- No firmware asset conversion or packed C registry yet.
- No compositor implementation yet.
- No simulator recipe renderer yet.
- No peripheral/environment asset recipe yet.
