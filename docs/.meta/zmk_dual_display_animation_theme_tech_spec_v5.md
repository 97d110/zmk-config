# Dual nice!view Animation Theme Tech Spec v5

This document is the v5 planning spec for the Eyelash Sofle dual nice!view
animation system. It supersedes the older animation-state requirement prompts
where they describe typing as multiple core activity buckets. The current core
model is intentionally lean: core activity is only `idle`, `typing`, or
`sleep`. Any visual typing intensity, elapsed typing phase, hold, settle, or
decay behavior belongs in the animation theme layer behind the renderer
boundary.

## 1. Project Conventions

### Repository Boundaries

- `build.yaml` is the release contract. Build target changes must keep artifact
  names and docs aligned.
- `config/west.yml` stays minimal and pinned to ZMK `v0.3`. Do not add donor
  display repos as runtime dependencies.
- Hardware behavior changes require checking both
  `boards/arm/eyelash_sofle/` and `config/`.
- Normal firmware artifacts must not gain debug USB logging snippets. Runtime
  display diagnostics belong in the dedicated debug artifacts.
- `keymap-drawer/*.svg` and `keymap-drawer/*.yaml` are generated outputs.
- Do not edit workspace dependencies such as `.zmk/`, `zmk/`, `modules/`, or
  `.west/`.

### Display Engine Boundaries

The display engine is split into durable product code and temporary/mock code.

- `display/core/` owns LVGL-free state, mapping, dimensions, status planning,
  scene planning, and generic animation-region contracts.
- `display/firmware/` owns ZMK runtime adapters, event subscriptions, semantic
  typing lifecycle, and render queueing.
- `display/render/lvgl/` owns the LVGL status-screen entry point, viewport
  mapping, and renderer contract.
- `display/assets/` is reserved for durable generated assets and registries.
- `display/mock/` owns temporary placeholder rendering. It must remain
  deletable once the real theme renderer exists.
- `sim/` owns host-only preview tooling. It may render approximations, but it
  must use the durable core planner instead of duplicating planner behavior.

Durable APIs must use generic behavior-oriented names such as `scene_variant`,
`status_slot`, `animation_plan`, `state_bucket`, and `screen_plan`. Final art
names, story names, donor repo names, fixture names, and placeholder asset names
must not leak into `display/core/`.

### Physical Display Contract

The nice!view displays are treated as portrait rectangles:

- width: 68 px,
- height: 160 px,
- top and bottom edges are short,
- left and right edges are long,
- the 14 px status bar lives across the narrow top edge,
- the animation region starts below the status bar and is 68 px by 146 px.

Any renderer, simulator, asset generator, or viewport mapper must preserve this
contract.

### Logging Convention

- Use the single firmware logging module `zmk_dual_display`.
- Display-engine files include `display/log.h` and use the
  `ZMK_DUAL_DISPLAY_LOG_*` wrappers except for the module registration point.
- Use debug logs for state mapping, scene selection, plan construction, renderer
  selection, and useful state transitions.
- Use warnings for recoverable fallbacks, invalid enum values, missing state, or
  null output buffers.
- Do not add unconditional logs to timer, animation-frame, or redraw loops.
- Debug logging must be observable through dedicated display-engine debug
  artifacts, not normal builds.

## 2. What Is Already Done

### Build And Wiring Baseline

- The repo is exposed as a Zephyr board root through `zephyr/module.yml`.
- Left and right Eyelash Sofle boards both enable ZMK display support.
- The left half is the split central.
- The build matrix includes normal, Studio, reset, USB debug, and
  display-engine debug artifacts.
- The local display engine disables upstream `NICE_VIEW_WIDGET_STATUS` when the
  engine is enabled so this repo supplies the single
  `zmk_display_status_screen()` implementation.

### Durable Core And Planning

- `display/core/dual_display_state.*` defines shared display state:
  - side,
  - role,
  - battery bucket with charging variants,
  - semantic activity: `idle`, `typing`, `sleep`,
  - transport,
  - split-link state,
  - layer mode.
- `display/core/dual_display_plan.*` turns state into:
  - a portrait top status bar plan,
  - a lower animation-region plan,
  - scene kind: `normal`, `sleep`, `link_error`, or `fallback`,
  - scene variant,
  - semantic activity,
  - layer mode,
  - energy level,
  - charging flag.
- Scene selection already applies the durable override order:
  - sleep overrides everything,
  - disconnected split link overrides normal scenes,
  - invalid enum state falls back,
  - otherwise the scene is normal with modifiers carried forward.

### Current Firmware State Pipeline

- `display/firmware/dual_display_state_adapter.c` initializes state from ZMK
  runtime sources and stores one firmware display state behind a mutex.
- ZMK event subscriptions cover activity, battery, USB, endpoint, BLE profile,
  keycode, layer, and split peripheral status where those APIs are available.
- Central-only APIs are guarded so peripheral builds do not link central-only
  objects.
- Render refreshes are queued on ZMK's display work queue only when visual state
  changes.
- `docs/display-firmware-animation-flow.md` documents the runtime wiring and
  event lifecycle.

### Lean Typing Core

- keypress events take the hot path,
- the hot path sets semantic display activity to `typing` and marks whether
  the current configurable period had a key,
- `typing_activity_work` runs on
  `CONFIG_ZMK_DUAL_DISPLAY_TYPING_CHECK_PERIOD_MS`,
- a period with keypresses keeps core activity at `typing`,
- a quiet period returns core activity to `idle`,
- complete state logs occur once per period,
- core does not expose typing elapsed time, WPM, frame index, decay phase, or
  theme intensity.

Theme behavior may derive richer animation phases from core transitions, but
that derived state must stay behind the renderer/theme boundary.

### Renderer And Simulator

- `display/render/lvgl/dual_display_status_screen.c` owns the ZMK display entry
  point and calls the renderer contract.
- `display/render/lvgl/viewport.*` owns portrait-to-framebuffer mapping.
- `display/render/lvgl/screen_renderer.h` is the durable LVGL renderer
  contract.
- `display/mock/lvgl/placeholder_renderer.c` currently implements the contract
  with temporary placeholder visuals.
- `sim/` provides host console and web previews using the durable core planner.
- `make sim-build` and `make verify` are the default local checks.

## 3. Final Vision

The final product is a synchronized dual-screen 1-bit animation system for the
Eyelash Sofle's two portrait nice!view displays.

### Creative Theme

The theme is a space narrative split across the keyboard:

| Role | Narrative Meaning | Default Visual |
|---|---|---|
| Central / Main | Actor | Meteor, fireball, energy body, glitching subject |
| Peripheral | POV / Environment | Planet, horizon, starfield, nebula, asteroid belt, event horizon |

The two displays should feel like one scene rendered through two role-specific
views. The main display usually shows the subject. The peripheral display
usually shows the destination or environment.

### Architecture

The final runtime path should be:

```text
ZMK runtime state
  -> lean core display state
  -> core screen plan
  -> theme animation controller
  -> scene recipe
  -> asset registry and compositor
  -> LVGL framebuffer renderer
  -> nice!view output
```

Core remains generic and stable. The animation theme layer owns expressive
policy:

- typing phase and visual intensity,
- post-typing hold and decay,
- frame clocks,
- role-specific scene recipe selection,
- asset variant selection,
- charging overlay cadence,
- procedural effect seeds,
- asset composition order.

### Theme State Contract

The theme should observe core screen plans and state transitions, then maintain
its own local animation context:

```c
struct zmk_dual_display_theme_context {
    enum zmk_dual_display_side side;
    enum zmk_dual_display_role role;
    uint32_t logical_tick;
    uint32_t scene_entered_tick;
    uint32_t typing_started_tick;
    uint32_t typing_last_active_tick;
    uint8_t typing_phase;
    uint8_t visual_intensity;
    uint8_t decay_phase;
};
```

This shape is illustrative, not a required ABI. The important rule is that
these fields are theme-local and do not become `display/core/` state.

### Theme Interpretation Of Lean Core Activity

Core activity maps to theme behavior like this:

| Core Activity | Theme Responsibility |
|---|---|
| `sleep` | Immediately choose sleep/black scene. No animation clock needed. |
| `idle` | Choose idle loop, or continue theme-local settle/decay if the previous state was typing. |
| `typing` | Start or continue a local typing animation timeline and raise visual intensity according to theme policy. |

The old 2s, 5s, 10s, and 15s typing levels become theme-local phases, for
example:

| Theme Phase | Possible Derivation | Visual Meaning |
|---|---|---|
| `typing_light` | first active period or low intensity | early descent, sparse stars |
| `typing_medium` | sustained typing | visible tail, planet grows |
| `typing_high` | longer sustained typing | long tail, heavy streaks |
| `typing_peak` | max sustained intensity | reentry/fireball, horizon zoom |

The phase derivation can use the theme context's tick counters, repeated
`typing` periods, frame count, or future animation timer signals. It must not
require core activity enums to grow.

### Composition Rules

The firmware should avoid storing the full cartesian product as flattened
frames:

```text
activity x layer x battery x charging x role x frame
```

Instead, scenes should be composed from:

- background sequences,
- actor sprite sheets,
- effect overlays,
- masks,
- procedural effects,
- limited full-frame sequences only when composition is not practical.

Scene selection priority remains:

1. sleep scene,
2. split-link error scene,
3. core activity interpreted by theme context,
4. layer-mode modifier,
5. battery energy modifier,
6. charging overlay,
7. role and side variant.

Transport state is status-bar information and must not multiply the animation
asset matrix.

## 4. Development Roadmap

Each increment below must be deployable, debuggable, simulatable, and testable.
Each increment must also create a handoff document under `.agentic/context/`
using a name such as:

```text
.agentic/context/display-engine-increment-6.md
```

Every handoff must explain:

- what was implemented,
- how it was implemented,
- why the boundary was chosen,
- what is durable and what is temporary,
- how to simulate it,
- how to debug it on firmware,
- what validation was run,
- what remains intentionally incomplete.

### Increment 6: Theme Boundary And No-Asset Theme Stub

Goal: introduce the real theme boundary without replacing the mock renderer yet.

Implement:

- a durable renderer/theme interface behind `display/render/lvgl/`,
- a theme-local context object that can observe core plans,
- a no-asset theme stub that maps `normal`, `sleep`, `link_error`, and
  `fallback` scenes to simple generic draw commands or placeholder calls,
- debug logs for scene entry and theme context changes, gated by state changes.

Deployable:

- normal display-engine firmware still renders placeholders,
- no asset files required.

Debuggable:

- display-engine debug artifact logs theme scene entry and context transitions.

Simulatable:

- simulator can show the same selected scene and theme context summary.

Testable:

- `make verify`,
- `make sim-build`,
- scripted simulator cases for sleep, link error, typing, idle, and layer
  changes.

Handoff:

- `.agentic/context/display-engine-increment-6.md`.

### Increment 7: Theme-Local Typing Phase And Decay

Goal: reintroduce expressive typing progression as theme policy, not core
state.

Implement:

- theme-local typing phases such as light, medium, high, and peak,
- configurable or hard-coded initial thresholds local to the theme,
- post-typing settle/decay while core has returned to `idle`,
- simulator controls that can advance ticks and show phase transitions,
- logs only on phase changes.

Deployable:

- firmware continues to use placeholder or simple theme visuals.

Debuggable:

- complete firmware state logs still show only core `typing` or `idle`,
- theme logs show derived phase and decay.

Simulatable:

- simulator can exercise sustained typing periods and quiet decay.

Testable:

- host tests or scripted simulator batches for phase progression,
- `make verify`,
- `make sim-build`.

Handoff:

- `.agentic/context/display-engine-increment-7.md`.

### Increment 8: Asset Manifest Schema And Validation

Goal: define the durable asset contract before adding final art.

Implement:

- `display/assets/schema/` or equivalent manifest documentation,
- manifest fields for asset id, type, dimensions, frame count, loop mode, masks,
  anchor, blend mode, role, and tags,
- a validation script for 1-bit PNG inputs and manifest consistency,
- a tiny fixture set under a clearly temporary path, or a durable sample if it
  is intended to survive.

Deployable:

- firmware behavior does not depend on the new assets yet.

Debuggable:

- validation output identifies bad dimensions, gray pixels, alpha, missing
  masks, and frame-count mismatches.

Simulatable:

- simulator can list known assets or load fixture metadata.

Testable:

- `make verify` includes schema or manifest checks,
- validator rejects intentionally invalid fixture inputs if fixtures are added.

Handoff:

- `.agentic/context/display-engine-increment-8.md`.

### Increment 9: Asset Conversion Pipeline And C Registry

Goal: convert source art into firmware-ready 1-bit packed assets.

Implement:

- source PNG to packed bitmap conversion,
- generated C or header registry under `display/assets/generated/`,
- clear generated-output policy,
- mask pairing for sprites,
- blend-mode metadata in generated registry entries.

Deployable:

- the generated registry can compile without being rendered yet.

Debuggable:

- conversion script reports asset ids, dimensions, bytes, masks, and warnings.

Simulatable:

- simulator can load the same generated registry or a host-readable equivalent.

Testable:

- conversion script unit or fixture checks,
- `make verify`,
- `make sim-build`.

Handoff:

- `.agentic/context/display-engine-increment-9.md`.

### Increment 10: 1-Bit Compositor

Goal: create the rendering primitive that composes sprites, masks, and overlays.

Implement:

- framebuffer abstraction for 68 x 146 animation region,
- `copy_with_mask`, `or_white`, `clear_black`, `xor_mask`, and
  `invert_region` blend modes,
- clipping at region boundaries,
- optional full-screen 68 x 160 path for sleep/error scenes if needed,
- host tests for composition results.

Deployable:

- renderer can still display placeholder scenes if assets are incomplete.

Debuggable:

- state-change logs include selected recipe and asset ids, not every frame.

Simulatable:

- simulator can render compositor output as ASCII or web pixels.

Testable:

- deterministic host tests for masking, clipping, and blend modes,
- `make verify`,
- `make sim-build`.

Handoff:

- `.agentic/context/display-engine-increment-10.md`.

### Increment 11: Minimal Vertical Slice Asset Theme

Goal: prove the full theme path with the smallest useful asset set.

Implement only:

- one idle starfield background,
- one meteor sprite sheet with masks,
- one small tail effect,
- one charging bolt overlay,
- one peripheral planet or horizon asset,
- one link-error/glitch mask.

Scenarios:

- idle low energy,
- idle high energy,
- typing low energy,
- typing high energy charging,
- split disconnected,
- central and peripheral role variants.

Deployable:

- firmware renders a real but incomplete asset theme.

Debuggable:

- display-engine debug artifacts show recipe selection and missing-asset
  fallbacks.

Simulatable:

- simulator previews the same minimal scenes.

Testable:

- asset validator,
- compositor tests,
- simulator scripted scenarios,
- `make verify`,
- `make sim-build`.

Handoff:

- `.agentic/context/display-engine-increment-11.md`.

### Increment 12: Layer Flavors As Scene Recipe Modifiers

Goal: add Symbol, Mod, and Config visual flavors without multiplying the base
asset matrix.

Implement:

- Symbol layer modifier: binary tail, digital debris, nebula/data-cloud
  peripheral background,
- Mod layer modifier: satellite obstacles, mechanical asteroid belt,
- Config layer modifier: inverted actor, warp tunnel, event horizon,
- unknown layer fallback.

Deployable:

- each layer has a visible theme difference even with limited assets.

Debuggable:

- logs identify selected layer modifier and fallback decisions.

Simulatable:

- simulator can switch layers and capture central/peripheral differences.

Testable:

- recipe selection tests,
- asset validation,
- `make verify`,
- `make sim-build`.

Handoff:

- `.agentic/context/display-engine-increment-12.md`.

### Increment 13: Battery, Charging, And Energy Polish

Goal: make energy and charging modifiers expressive but compositional.

Implement:

- low, medium, and high energy sprite or mask variants,
- charging overlay cadence,
- battery-driven glow/tail/atmosphere differences,
- no transport-driven animation variants.

Deployable:

- firmware displays meaningful energy differences.

Debuggable:

- logs show energy bucket and charging overlay state on changes.

Simulatable:

- simulator exercises battery percent and charging states.

Testable:

- recipe modifier tests,
- asset coverage validation,
- `make verify`,
- `make sim-build`.

Handoff:

- `.agentic/context/display-engine-increment-13.md`.

### Increment 14: Timing, Synchronization, And Frame Clock

Goal: make both halves feel synchronized without tight coupling to unstable
firmware timing.

Implement:

- shared logical frame clock policy,
- compatible loop lengths such as 12, 18, and 24 frames,
- role-specific frame offsets only when intentional,
- no per-frame logs,
- simulator playback at target timing.

Deployable:

- firmware animations loop coherently on both halves.

Debuggable:

- logs expose scene and clock resets, not frame ticks.

Simulatable:

- simulator can play, pause, step, and capture frames.

Testable:

- deterministic frame-index tests,
- simulator playback smoke test,
- `make verify`,
- `make sim-build`.

Handoff:

- `.agentic/context/display-engine-increment-14.md`.

### Increment 15: Full Theme Expansion And Asset Freeze

Goal: complete the final visual library after the engine path is proven.

Implement:

- expanded meteor, tail, planet, nebula, asteroid, warp, event-horizon, charging,
  and glitch assets,
- final manifests,
- missing-asset fail-fast validation for release assets,
- asset-size review for firmware storage,
- generated asset docs.

Deployable:

- normal display-engine artifacts use the final theme.

Debuggable:

- debug artifacts expose recipe and fallback logs.

Simulatable:

- simulator can preview all required scenes and export reference frames.

Testable:

- complete manifest validation,
- compositor and recipe tests,
- simulator scenario suite,
- `make verify`,
- firmware build validation through GitHub Actions.

Handoff:

- `.agentic/context/display-engine-increment-15.md`.

## 5. Animation Asset Guide

### Target Format

Source art should be generated as PNGs but firmware should consume converted
1-bit packed bitmap data, not runtime PNG decoding.

Required source constraints:

```text
Full display: 68 px wide x 160 px high
Animation region: 68 px wide x 146 px high
Status bar: 68 px wide x 14 px high
Color: pure 1-bit only
Black: #000000
White: #FFFFFF
No gray
No antialiasing
No semitransparent pixels
No alpha gradients
```

Dithering is allowed only as actual black and white pixels.

Allowed:

- 2 x 2 Bayer dithering,
- ordered dithering,
- Floyd-Steinberg-style black and white noise,
- hand-authored 1 px texture.

Rejected:

- gray pixels,
- antialiased edges,
- alpha gradients,
- semitransparent shadows,
- full-color source that has not been validated after conversion.

### Asset Types

Prefer reusable assets:

| Type | Use |
|---|---|
| `background_sequence` | Starfield, nebula, warp base, event horizon base |
| `sprite_sheet` | Meteor, planet, satellites, fragments |
| `effect_sheet` | Tail, flame, charging bolts, binary particles, glitch strips |
| `mask_sheet` | Transparency masks, crack masks, tear masks |
| `procedural_descriptor` | Star positions, drift speed, flicker seed, line seeds |
| `full_frame_sequence` | Sleep void, heavy reentry, tunnel, no-signal scene when composition is not practical |

Full-frame sequences should be rare. They are acceptable for scenes where
composition would cost more firmware complexity or storage than it saves.

### Recommended Source Tree

Use a source-art tree that keeps original PNGs separate from generated firmware
assets:

```text
animation_sources/
  actors/
    meteor/
      low/
      medium/
      high/
  effects/
    tails/
    charging/
    glitches/
    binary/
  backgrounds/
    starfield/
    nebula/
    asteroid_belt/
    warp/
    event_horizon/
  full_sequences/
    sleep_void/
    reentry_fireball/
    no_signal_error/
  manifests/
    assets_manifest.json
    scenes_manifest.json
```

Generated firmware outputs should live under a generated boundary such as:

```text
display/assets/generated/
```

Do not hand-edit generated C or header files. Update source PNGs, manifests, or
conversion scripts, then regenerate.

### Naming Rules

Asset names may use theme words inside manifests and generated registries,
because those live in the theme/asset layer. Durable `display/core/` APIs must
remain generic.

Good asset ids:

- `meteor_rotation_high`,
- `tail_flame_medium`,
- `starfield_slow`,
- `planet_horizon_high`,
- `charging_bolt_variants`,
- `glitch_horizontal_tears`.

Avoid names that encode the full matrix:

- `typing_10s_symbol_high_charging_left_frame_00`,
- `right_mod_51_100_usb_connected_scene`,
- `layer3_battery_high_typing_peak_peripheral_full`.

Use recipe manifests to combine generic dimensions:

- activity-derived theme phase,
- layer flavor,
- energy,
- charging overlay,
- role variant.

### Sprite And Mask Contract

Reusable sprites should provide pixels and masks:

```text
pixels bitmap
mask bitmap
```

Compositor behavior:

```c
framebuffer = (framebuffer & ~mask) | (pixels & mask);
```

Recommended metadata:

```json
{
  "asset_id": "meteor_rotation_high",
  "type": "sprite_sheet",
  "width": 32,
  "height": 32,
  "frame_count": 12,
  "anchor": { "x": 16, "y": 16 },
  "loop": "seamless",
  "blend_mode": "copy_with_mask",
  "role": "central",
  "tags": ["actor", "meteor", "energy_high"]
}
```

Blend modes:

| Mode | Behavior |
|---|---|
| `copy_with_mask` | Replace pixels under mask |
| `or_white` | Add white pixels only |
| `clear_black` | Clear pixels to black |
| `xor_mask` | Flicker, glitch, or invert masked region |
| `invert_region` | Invert a rectangular or masked region |

### Scene Recipe Manifest

Scene manifests should describe composition, not final flattened state folders.

Example:

```json
{
  "scene_id": "typing_high_type_central",
  "role": "central",
  "base_phase": "typing_high",
  "layer_flavor": "type",
  "background": {
    "asset_id": "starfield_fast",
    "motion": "down"
  },
  "actor": {
    "asset_id_by_energy": {
      "low": "meteor_rotation_low",
      "medium": "meteor_rotation_medium",
      "high": "meteor_rotation_high"
    },
    "path": "descending",
    "shake_px": 1
  },
  "effects": [
    {
      "asset_id": "tail_flame_long",
      "attach_to": "actor",
      "blend_mode": "or_white"
    }
  ],
  "charging_overlay": {
    "asset_id": "charging_bolt_variants",
    "cadence_frames": 2
  }
}
```

### First Asset Spike

Before generating the full visual library, create only:

- one idle starfield background,
- one meteor sprite sheet with masks,
- one small tail effect,
- one charging bolt overlay,
- one planet or horizon peripheral asset,
- one error or glitch mask.

Use this set to prove:

- idle low energy,
- idle high energy,
- typing low energy,
- typing high energy charging,
- split disconnected,
- central versus peripheral role variants.

Do not generate the full matrix until the manifest, converter, compositor, and
simulator all work together.

### Generator Acceptance Checklist

Accept generated source art only when:

- dimensions match the manifest,
- every pixel is pure black or pure white after validation,
- masks exist for masked sprites and match frame count and dimensions,
- seamless loops actually loop,
- the status bar area is preserved unless the asset is explicitly full-screen,
- central and peripheral assets share compatible timing,
- frame counts are compatible with the theme clock,
- source art is separated from generated firmware assets.

Reject and regenerate when:

- gray or alpha pixels exist,
- antialiasing is visible,
- masks and pixels are misaligned,
- a reusable sprite was delivered only as flattened full frames,
- the asset duplicates a whole state combination that could be expressed as a
  recipe plus modifiers,
- the art treats the display as landscape instead of portrait.
