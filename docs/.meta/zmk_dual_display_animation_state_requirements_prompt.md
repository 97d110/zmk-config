# Codex Prompt: Dual nice!view Animation State Requirements for ZMK

You are working in this repository:

```text
https://github.com/97d110/zmk-config
```

This project is a ZMK keyboard configuration for a split keyboard with two nice!view displays. The goal is to build a custom dual-display animation engine where each half can render a different but synchronized visual role.

This prompt defines the **firmware/state-engine requirements** for the animation system. Do not implement the final visual art yet unless explicitly requested. First make sure the engine can support the needed state combinations cleanly.

---

## Core Goal

Build a state-driven animation system where the display output is determined by keyboard state, not by hardcoded one-off full-screen images.

The final visual theme is a dual-screen “meteor approaching planet” story:

- One shield acts as the **Main / Actor** display.
- The other shield acts as the **Peripheral / POV / Environment** display.
- Both displays should feel synchronized.
- The firmware should avoid storing every possible final state combination as full-frame assets.
- Prefer reusable sprites, masks, overlays, procedural effects, and only limited full-frame sequences where truly needed.

---

## Desired Architecture

Use this shape:

```text
keyboard/ZMK runtime state
        ↓
normalized dual-display state
        ↓
scene/state planner
        ↓
draw plan / scene recipe
        ↓
renderer backend
        ↓
nice!view output
```

Keep these boundaries:

```text
display/core/
  Pure state, mapping, planning, scene selection.
  No LVGL-specific assumptions.
  Avoid final-art-specific names in durable engine logic where possible.

display/render/lvgl/
  LVGL rendering boundary.
  Receives a plan and renders it.

display/mock/
  Temporary placeholder renderer and placeholder visuals.

display/assets/
  Durable reusable assets and generated asset registries.
```

The final animation theme should be represented as data-driven scene recipes and reusable assets, not hardcoded renderer branches.

---

## Required State Inputs

The planner must reason about these dimensions.

### 1. Display Side

```c
enum zmk_dual_display_side {
    ZMK_DUAL_DISPLAY_SIDE_LEFT,
    ZMK_DUAL_DISPLAY_SIDE_RIGHT,
};
```

Both halves must be supported.

Do not assume long-term that “left” always means main actor and “right” always means peripheral environment. The current implementation may map primary/secondary by side, but the design should remain flexible enough to support role-based mapping later.

---

### 2. Display Role

```c
enum zmk_dual_display_role {
    ZMK_DUAL_DISPLAY_ROLE_UNKNOWN,
    ZMK_DUAL_DISPLAY_ROLE_CENTRAL,
    ZMK_DUAL_DISPLAY_ROLE_PERIPHERAL,
};
```

| Role | Visual Meaning |
|---|---|
| Central / Main | Actor display, usually the meteor. |
| Peripheral | POV / Environment display, usually the planet, horizon, nebula, or destination. |
| Unknown | Safe fallback rendering. |

Preserve role as a first-class concept even if the first implementation still maps variants by side.

---

### 3. Activity Bucket

```c
enum zmk_dual_display_activity_bucket {
    ZMK_DUAL_DISPLAY_ACTIVITY_IDLE,
    ZMK_DUAL_DISPLAY_ACTIVITY_SLEEP,
    ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_2S,
    ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_5S,
    ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_10S,
    ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_15S,
};
```

| Activity | Expected Meaning |
|---|---|
| Sleep | Screen black/off or ultra-minimal. |
| Idle | Calm loop. Meteor floats/tumbles slowly. Planet remains distant/static. |
| Typing 2s | Light movement, early descent/approach. |
| Typing 5s | Medium activity, visible tail, planet grows. |
| Typing 10s | High activity, long tail, strong star streaks / fast approach. |
| Typing 15s | Peak activity, reentry/fireball/rapid horizon approach. |

Treat these as activity buckets, not necessarily unique full-frame folders.

---

### 4. Layer Mode

```c
enum zmk_dual_display_layer_mode {
    ZMK_DUAL_DISPLAY_LAYER_UNKNOWN,
    ZMK_DUAL_DISPLAY_LAYER_TYPE,
    ZMK_DUAL_DISPLAY_LAYER_SYMBOL,
    ZMK_DUAL_DISPLAY_LAYER_MOD,
    ZMK_DUAL_DISPLAY_LAYER_CONFIG,
};
```

| Layer | Visual Meaning |
|---|---|
| Type | Default meteor/planet typing animation. |
| Symbol | Digital/binary particle flavor, nebula/gas texture. |
| Mod | Satellites, mechanical asteroid belt, obstacles. |
| Config / Control | Inverted warp tunnel / event horizon / high-contrast radial lines. |
| Unknown | Safe fallback rendering. |

Layer mode should usually modify a base scene rather than force a full replacement of every asset.

---

### 5. Battery Bucket

```c
enum zmk_dual_display_battery_bucket {
    ZMK_DUAL_DISPLAY_BATTERY_UNKNOWN,
    ZMK_DUAL_DISPLAY_BATTERY_0_10,
    ZMK_DUAL_DISPLAY_BATTERY_11_50,
    ZMK_DUAL_DISPLAY_BATTERY_51_100,
    ZMK_DUAL_DISPLAY_BATTERY_0_10_CHARGING,
    ZMK_DUAL_DISPLAY_BATTERY_11_50_CHARGING,
    ZMK_DUAL_DISPLAY_BATTERY_51_100_CHARGING,
};
```

| Battery Bucket | Animation Meaning |
|---|---|
| 0–10 | Low-energy variant. Small meteor, weaker glow, thinner tail. |
| 11–50 | Medium-energy variant. |
| 51–100 | Full-energy variant. Larger meteor, stronger/thicker dither glow. |
| Charging | Adds electric/plasma effect overlay. |
| Unknown | Safe fallback rendering. |

Battery should not force unique full-frame sequences. Prefer scaling, alternate sprites, dither masks, glow overlays, and charging effects.

---

### 6. Split Link State

```c
enum zmk_dual_display_split_link_state {
    ZMK_DUAL_DISPLAY_SPLIT_LINK_UNKNOWN,
    ZMK_DUAL_DISPLAY_SPLIT_LINK_CONNECTED,
    ZMK_DUAL_DISPLAY_SPLIT_LINK_DISCONNECTED,
};
```

| Split State | Animation Meaning |
|---|---|
| Connected | Normal animation. |
| Disconnected | Error visual: “NO SIGNAL”, broken meteor, static, cracked/glitched planet. |
| Unknown | Safe fallback or warning visual. |

Disconnected state should override normal scene selection.

---

### 7. Transport State

```c
enum zmk_dual_display_transport_state {
    ZMK_DUAL_DISPLAY_TRANSPORT_UNKNOWN,
    ZMK_DUAL_DISPLAY_TRANSPORT_USB,
    ZMK_DUAL_DISPLAY_TRANSPORT_BT,
    ZMK_DUAL_DISPLAY_TRANSPORT_DISCONNECTED,
};
```

Transport is primarily status-bar information. Do not let transport multiply the animation asset matrix.

---

## Required Scene Selection Priority

Scene selection should follow this priority:

```text
1. Sleep override
2. Split/link error override
3. Activity bucket
4. Layer mode modifier
5. Battery level modifier
6. Charging overlay
7. Side/role variant
```

Example:

```text
If sleeping:
  render sleep/black scene regardless of layer or battery.

If split disconnected:
  render error scene regardless of activity, but still allow side/role variation.

If typing for 10s on Symbol layer while charging:
  render high-energy typing scene
  apply Symbol visual flavor
  apply charging overlay
  apply battery intensity
  choose main/peripheral role variant
```

---

## Avoid Full Combination Explosion

Do not design around this:

```text
[ACTIVITY][LAYER][BATTERY][CHARGING][SIDE] -> unique folder of full-frame images
```

Design around this instead:

```text
state -> scene recipe -> reusable draw commands
```

A scene recipe may combine:

```text
background
actor sprite
effect sprite
motion path
procedural starfield
procedural glitch
battery modifier
charging overlay
status bar
```

---

## Desired Composable Rendering Model

A future scene recipe could conceptually look like:

```c
struct scene_recipe {
    enum scene_id scene;
    enum scene_variant variant;
    enum background_id background;
    enum actor_id actor;
    enum effect_id primary_effect;
    enum motion_profile_id motion;
    enum battery_visual_modifier battery_modifier;
    bool charging_overlay;
    bool inverted;
};
```

A lower-level draw plan could look like:

```c
struct draw_command {
    enum asset_id asset;
    int16_t x;
    int16_t y;
    uint8_t frame;
    enum blend_mode blend;
};
```

The renderer should know how to:

```text
clear framebuffer
draw bitmap/sprite
apply mask
draw procedural effect
flush frame
```

The renderer should not need to know what a meteor, planet, layer, or battery bucket means.

---

## Asset Strategy Requirements

### Reusable Sprites

Good candidates:

```text
meteor body
meteor rotations
tail shapes
charging bolts
satellite silhouettes
binary particles
small stars
nebula patches
warp-line segments
crack/glitch masks
planet surface texture chunks
```

### Procedural Effects

Good candidates:

```text
starfield drift
parallax
screen shake
1px vibration
charging bolt position changes
glitch horizontal offsets
warp-line placement
binary particle drift
simple dither/noise masks
```

### Limited Pre-rendered Sequences

Allowed for complex visuals:

```text
planet horizon zoom
event horizon tunnel
heavy re-entry fireball
dense nebula background
NO SIGNAL / broken meteor error scene
```

Use these sparingly.

---

## nice!view Asset Format Expectations

Source files may start as PNGs, but firmware should not rely on runtime PNG decoding.

Expected pipeline:

```text
source PNGs
  ↓
conversion script
  ↓
1-bit packed bitmap/sprite data
  ↓
C asset registry
  ↓
runtime renderer/compositor
```

Use pure 1-bit assets:

```text
Black: #000000
White: #FFFFFF
No gray
No alpha in final firmware data
```

Transparency should be represented by a mask, not runtime PNG alpha.

Recommended asset struct concept:

```c
struct bitmap_asset {
    const uint8_t *pixels;
    const uint8_t *mask;
    uint8_t width;
    uint8_t height;
    int8_t anchor_x;
    int8_t anchor_y;
};
```

Recommended blend modes:

```text
copy with mask
OR white pixels
clear black pixels
XOR glitch/flicker mask
invert region
```

---

## Frame Timing Requirements

- Both displays should share the same logical tick/frame clock where possible.
- Loops should use compatible lengths such as 12, 18, and 24 frames.
- Side-specific scenes may differ visually but should stay synchronized.
- Background starfield speed should match across both shields for the same activity state.
- The engine should support seamless loops.
- Do not block keyboard performance while animating.

---

## Status Bar Requirements

The status bar should remain separate from the animation region.

Status bar should support:

```text
battery
split link
transport
layer mode
```

The animation region should not overwrite the status bar unless a future full-screen mode is explicitly added.

---

## Required Fallbacks

Every state dimension needs safe fallback behavior.

Required fallbacks:

```text
unknown side -> normalized side or safe primary variant
unknown role -> safe side-based variant
unknown battery -> unknown battery icon + neutral animation modifier
unknown layer -> neutral/default visual
unknown transport -> unknown status icon
unknown split link -> warning/fallback icon
missing asset -> placeholder rectangle/checker/error glyph
```

No missing asset should crash the display engine.

---

## Recommended Implementation Order

1. Confirm current state model and planner boundaries.
2. Add/verify scene ID selection from activity + layer + sleep/split overrides.
3. Add an asset registry interface with fake assets.
4. Add a tiny compositor spike with one background, one actor sprite, one overlay.
5. Add battery modifier support without duplicating full scenes.
6. Add charging overlay support.
7. Add side/role variants.
8. Add simulator support to preview both displays.
9. Only then add real generated art assets.

---

## Acceptance Criteria

The engine is on the right path when:

- A small set of reusable assets can produce multiple visible states.
- Battery level changes the visual without requiring separate full-frame animations.
- Charging adds an overlay without duplicating the base scene.
- Layer mode changes scene flavor without duplicating every base frame.
- Main/peripheral displays can show different synchronized roles.
- Missing/unknown states render safe fallbacks.
- The renderer boundary remains generic.
- The story/theme is represented in scene data, not hardcoded renderer logic.

---

## Do Not Do

Avoid these:

```text
Do not generate every ACT/LAYER/BATTERY/SIDE combination as full-frame sequences.
Do not put meteor-specific logic deep inside the LVGL renderer.
Do not decode PNGs at runtime on the keyboard.
Do not make the status bar part of every animation frame asset.
Do not block typing or BLE behavior with display rendering.
Do not build the final art pipeline before proving the compositor/registry shape.
```

---

## Summary

The correct direction is:

```text
state-driven scene recipes
+ reusable sprite/effect assets
+ procedural rendering where cheap
+ limited pre-rendered sequences where necessary
+ generic renderer boundary
+ synchronized dual-display timing
```

The final visual story is important, but it should remain data/configuration consumed by the engine, not the architecture itself.
