# Codex Prompt: Dual nice!view Animation Asset Contract and Voyager Story Bible

You are working in this repository:

```text
https://github.com/97d110/zmk-config
```

The project is a ZMK split keyboard configuration with two nice!view displays. The final goal is a custom dual-screen animation system.

This prompt explains the **animation asset format**, the expected structure of generated animation inputs, and the creative “Voyager” story that should guide the generated visual assets.

Important: the firmware should not store every possible state combination as a unique full-screen animation if that can be avoided. The animation assets should be prepared so they can be composed from reusable sprites, masks, overlays, procedural effects, and only a small number of full-frame sequences when truly justified.

---

## High-Level Creative Direction

The animation theme is a dual-screen space narrative:

| Shield Role | Narrative Role | Primary Visual |
|---|---|---|
| Main / Central | The Actor | A meteor traveling, rotating, burning, glitching, or transforming. |
| Peripheral | The POV / Environment | The destination/environment: planet, starfield, nebula, asteroid belt, horizon, event horizon, static, etc. |

Together, both screens should feel like one synchronized scene split across the keyboard.

The **Main shield** should usually show the active subject.

The **Peripheral shield** should usually show where the subject is going or the environment it is moving through.

---

## Required Source Asset Format

Generated source art should follow these rules:

```text
Target display orientation: Portrait
Target full nice!view resolution: 68px wide × 160px high
Recommended animation region: 68px wide × 146px high if preserving a 14px status bar
Color mode: Pure 1-bit
Black: #000000
White: #FFFFFF
No gray
No antialiasing
No semi-transparent pixels
```

Dithering is allowed only by using actual black/white pixels:

```text
Allowed: 2×2 Bayer dithering, ordered dithering, Floyd-Steinberg-style black/white noise
Not allowed: gray pixels, alpha gradients, antialiasing
```

---

## Runtime Asset Expectation

The animation source files may be PNGs, but firmware should not rely on runtime PNG decoding.

Expected pipeline:

```text
source PNGs
  ↓
asset conversion script
  ↓
1-bit packed bitmap/sprite data
  ↓
C asset registry
  ↓
firmware compositor / renderer
```

---

## Preferred Asset Types

Do not assume every generated image is a final full-frame animation.

The preferred model is composable assets:

```text
backgrounds
sprites
sprite masks
effect overlays
procedural effect parameters
limited full-frame sequences
```

| Asset Type | Description | Example |
|---|---|---|
| Full-frame sequence | Complete 68×146 or 68×160 frame sequence. Use sparingly. | Event horizon tunnel, heavy reentry, NO SIGNAL scene. |
| Background tile/sequence | Background-only visual. | Starfield, nebula, warp tunnel base. |
| Actor sprite sheet | Reusable moving subject. | Meteor rotations, planet scale frames. |
| Effect sprite sheet | Reusable effect overlay. | Tail, flame, charging bolt, binary particles. |
| Mask | 1-bit transparency/overwrite mask. | Meteor transparent outline, glitch mask. |
| Procedural descriptor | Data-only description for generated runtime effect. | Star positions, drift speed, flicker seed. |

---

## Required Sprite/Mask Format

For reusable sprites, expect a pair of bitmaps:

```text
pixels bitmap
mask bitmap
```

The mask determines which pixels are active/transparent.

Conceptual compositor behavior:

```c
framebuffer = (framebuffer & ~mask) | (pixels & mask);
```

Recommended metadata per sprite:

```text
asset_id
width
height
anchor_x
anchor_y
frame_count
loop_mode
intended_blend_mode
```

Recommended blend modes:

| Blend Mode | Meaning |
|---|---|
| copy_with_mask | Replace pixels under mask. |
| or_white | Add white pixels only. |
| clear_black | Clear pixels to black. |
| xor_mask | Flicker/glitch/invert masked region. |
| invert_region | Control/event-horizon effect. |

---

## Recommended Folder/Input Layout

The animation generator may return PNGs in a layout like this:

```text
animation_sources/
  actors/
    meteor/
      l1_small/
        frame_000.png
        frame_001.png
        mask_000.png
        mask_001.png
      l2_medium/
      l3_large/
    planet/
      distant/
      approach_30/
      horizon_zoom/

  effects/
    tails/
      flame_short/
      flame_medium/
      flame_long/
      binary_tail/
    charging/
      bolt_variants/
    glitches/
      screen_tear/
      crack_masks/
    warp/
      radial_lines/
      star_streaks/

  backgrounds/
    starfield/
      idle/
      slow/
      medium/
      fast/
    nebula/
      symbol/
    asteroid_belt/
      mod/
    event_horizon/
      control/

  full_sequences/
    sleep_void/
    reentry_fireball/
    event_horizon_tunnel/
    no_signal_error/

  manifests/
    assets_manifest.json
    scenes_manifest.json
```

The exact layout can change, but reusable subjects/effects must be separated instead of only providing final flattened state combinations.

---

## Example Asset Manifest

```json
{
  "asset_id": "meteor_l3_rotation",
  "type": "sprite_sheet",
  "resolution": {
    "width": 32,
    "height": 32
  },
  "frame_count": 12,
  "pixels": [
    "actors/meteor/l3_large/frame_000.png",
    "actors/meteor/l3_large/frame_001.png"
  ],
  "masks": [
    "actors/meteor/l3_large/mask_000.png",
    "actors/meteor/l3_large/mask_001.png"
  ],
  "anchor": {
    "x": 16,
    "y": 16
  },
  "loop": "seamless",
  "blend_mode": "copy_with_mask"
}
```

---

## Example Scene Manifest

```json
{
  "scene_id": "T10_TYP",
  "role": "main",
  "frame_count": 24,
  "background": {
    "asset_id": "starfield_fast",
    "motion": "downward_fast"
  },
  "actor": {
    "asset_id": "meteor_l3_rotation",
    "motion_path": "descending",
    "shake_px": 1
  },
  "effects": [
    {
      "asset_id": "flame_tail_long",
      "attach_to": "meteor",
      "flicker": true
    },
    {
      "asset_id": "star_streaks_fast",
      "blend_mode": "or_white"
    }
  ],
  "battery_modifiers": {
    "l1": {
      "actor_scale": "small",
      "tail_density": "low"
    },
    "l2": {
      "actor_scale": "medium",
      "tail_density": "medium"
    },
    "l3": {
      "actor_scale": "large",
      "tail_density": "high"
    }
  },
  "charging_overlay": {
    "asset_id": "charging_bolt_variants",
    "frequency_frames": 2
  }
}
```

---

## Animation State Tree

Treat these rows as **scene recipes**, not necessarily unique final full-frame folders.

| Animation UID | Activity | Layer | Battery | Recommended Frames | Main Shield Description — Actor | Peripheral Shield Description — POV |
|---|---:|---|---|---:|---|---|
| SLP_VOID | Sleep | Any | Any | 1 | Total black screen. | Total black screen. |
| IDL_ANY | Idle | Any/default | L1–L3 | 12 | Meteor tumbles slowly in place. No tail. | Distant static starfield with a faint planet dot. |
| T02_TYP | 2s typing | Type | L1–L3 | 15 | Meteor begins descending. Single-pixel stars pass. | Distant planet dot grows slightly. Star drift. |
| T05_TYP | 5s typing | Type | L1–L3 | 18 | Small dithered tail appears. Meteor vibrates 1px. | Planet sphere becomes visible at the bottom. |
| T10_TYP | 10s typing | Type | L1–L3 | 24 | Long flickering flame. Heavy star streaks. | Planet fills ~30% of screen. Surface detail visible. |
| T15_TYP | 15s typing | Type | L1–L3 | 24 | Entry: meteor engulfed in white fireball. | Approach: rapid zoom into planet horizon. |
| T05_SYM | 5s+ typing | Symbol | L1–L3 | 18 | Tail becomes binary 0/1 digital particles. | Background becomes thick dithered gas nebula. |
| T05_MOD | 5s+ typing | Mod | L1–L3 | 18 | Meteor dodges orbiting satellite silhouettes. | View of massive mechanical asteroid belt. |
| T10_CTR | 10s+ typing | Config/Control | L1–L3 | 24 | Inverted: black meteor in white warp tunnel. | Inverted: event horizon with radial lines. |
| ERR_LOST | Split lost/error | Any | Any | 10 | Meteor breaks; “NO SIGNAL” text flickers. | Static/snow effect with cracked/glitched planet. |

---

## Story Subject Profiles

| Subject | Primary Role | Detail & Motion | 1-Bit Aesthetic |
|---|---|---|---|
| Meteor | Main | Protagonist. Jagged, rotating, descending, sometimes vibrating or breaking. | L1: tiny pebble. L2: mid rock. L3: glowing boulder. Use hard jagged silhouette with dithered edge/glow. |
| Tail | Main | Represents speed/activity/WPM intensity. Longer and denser at higher activity. | Bayer/ordered dithering for flame density. Can be short, medium, long, binary, or plasma. |
| Planet | Peripheral | Looming destination. Grows over time as activity increases. | Circular dithering to show curvature/shadow. Atmospheric ring gets stronger with battery. |
| Starfield | Both | Background continuity between screens. Speed increases with activity. | Single-pixel stars, sparse at idle, streaked at high activity. |
| Nebula | Peripheral / Symbol | Environmental texture for Symbol layer. | Floyd-Steinberg-like 1-bit cloudy noise patches. |
| Plasma Arcs | Both | Charging-only overlay. Electric bolts or arcs flicker around actor/environment. | Jagged 1px lines. Position should change every ~2 frames. |
| Satellites | Both / Mod | Obstacles for Mod/utility state. | Solid black/white hard-edged geometric silhouettes. |
| Asteroid Belt | Peripheral / Mod | Mechanical/environmental variation for Mod layer. | Hard-edged chunks, parallax bands, sparse dither. |
| Warp Lines | Both / Control | High-speed control/config layer. | Extreme contrast long radial lines, possibly inverted. |
| Event Horizon | Peripheral / Control | Destination for Control layer. | Radial white lines, black center, strong inversion. |
| Glitch/Static | Both / Error | Split connection lost or unknown. | Snow, tearing, horizontal offsets, cracked masks. |

---

## Subject-State Variability

### Battery Level

| Battery | Main Shield Meteor | Peripheral Shield Planet |
|---|---|---|
| L1 / Low | Small meteor, about 8px if possible. Weak/no glow. Sparse tail. | Thin or nearly absent atmospheric glow. |
| L2 / Medium | Medium meteor. Moderate glow/tail density. | Medium dithered atmospheric ring. |
| L3 / High | Large meteor, up to ~32px if visually appropriate. Strong dithered glow. | Thicker/brighter dithered atmospheric glow. |
| Charging | Add electric bolt/plasma overlay. | Add electric/plasma arcs or atmospheric flicker. |

Battery should usually be implemented through alternate sprite scale, alternate dither mask, overlay effect, or metadata-driven modifier.

---

### Activity Level

| Activity | Main Motion | Peripheral Motion | Background Speed |
|---|---|---|---|
| Idle | Slow tumble, no descent, no tail. | Static planet dot / calm distant scene. | Very slow or static. |
| Typing 2s | Begin descent. | Slight planet growth. | Slow star drift. |
| Typing 5s | Descending with small vibration. | Planet partially visible. | Medium drift. |
| Typing 10s | Fast descent, long tail, strong streaks. | Planet fills ~30%. | Fast streaks. |
| Typing 15s | Fireball/reentry. | Horizon zoom. | Very fast / dramatic. |
| Sleep | No animation. | No animation. | None. |

---

### Layer Mode

| Layer | Main Shield Modifier | Peripheral Shield Modifier |
|---|---|---|
| Type | Default meteor/flame/descent. | Default planet/starfield approach. |
| Symbol | Binary particle tail, digital debris. | Dithered nebula / data-cloud feeling. |
| Mod | Satellite obstacles, mechanical interference. | Mechanical asteroid belt / orbital structures. |
| Config/Control | Inverted meteor, warp tunnel. | Event horizon / radial control tunnel. |
| Unknown | Neutral fallback. | Neutral fallback. |

Layer should usually be a scene recipe modifier, not a total asset replacement.

---

### Charging

Charging is an overlay, not a separate full scene.

Requirements:

```text
1px electric bolt or plasma arc
position changes every ~2 frames
can appear around meteor, planet, or screen edge
must work on top of multiple base scenes
```

Recommended implementation:

```text
charging_bolt_variants sprite sheet
or procedural jagged-line renderer
or both
```

---

### Split Connection Error

When split link is lost, error visuals should override normal animation.

| Role | Error Visual |
|---|---|
| Main | Meteor cracks/breaks apart. “NO SIGNAL” flickering text. |
| Peripheral | Static/snow, cracked planet, horizontal tearing/glitch. |

---

## Loop and Synchronization Rules

```text
last frame transitions seamlessly to first frame
frame counts should be compatible: 12, 18, 24, etc.
both shields should use the same logical frame clock
starfield/background drift speed should match across both shields for the same activity
side-specific assets may differ, but timing should feel synchronized
```

---

## Recommended First Asset Spike

Before creating the full art library, generate only a tiny test set:

```text
1 idle starfield background
1 meteor sprite sheet with masks
1 small tail effect
1 charging bolt overlay
1 planet/dot peripheral sprite
1 error/glitch mask
```

Use this set to prove:

```text
idle L1
idle L3
typing L1
typing L3 charging
split disconnected
main vs peripheral variants
```

Do not generate the full final matrix until the compositor, registry, and simulator are proven.

---

## Prompt Template for Future Animation Generation

```markdown
# Generate 1-bit nice!view Animation Assets

## Target

Generate source PNG assets for a ZMK nice!view keyboard display animation.

Resolution:
- Full display: 68px wide × 160px high, portrait
- Animation region: 68px wide × 146px high, leaving 14px top status bar unless stated otherwise

Color:
- Pure black #000000
- Pure white #FFFFFF
- No gray
- No antialiasing
- No alpha gradients
- 1-bit dithering only

## Asset Type

Generate: [sprite sheet / background sequence / effect overlay / mask / full-frame sequence]

Asset ID:
[INSERT ASSET ID]

Frame count:
[INSERT FRAME COUNT]

Loop:
[seamless / one-shot / ping-pong]

Role:
[main actor / peripheral environment / both]

## Visual Description

Subject:
[meteor / tail / planet / starfield / nebula / satellite / warp / glitch / etc.]

State context:
[activity bucket, layer mode, battery modifier, charging or not]

Motion:
[describe movement, vibration, drift, zoom, parallax, flicker]

Dithering:
[ordered Bayer / circular dither / cloudy noise / no dithering]

Mask requirement:
[yes/no]
If yes, provide matching mask frames named mask_000.png, mask_001.png, etc.

## Output Format

Return files in a folder containing:

- frame_000.png
- frame_001.png
- ...
- mask_000.png if applicable
- mask_001.png if applicable
- asset_manifest.json

The last frame must transition cleanly back to the first frame if loop is seamless.
```

---

## Do Not Accept From Animation Generator

Reject or regenerate assets if they contain:

```text
gray pixels
anti-aliased edges
semi-transparent pixels
wrong resolution without metadata
no masks for sprites that need transparency
full-screen flattened combinations when reusable sprites were requested
unloopable sequences when seamless loop was requested
text too small or unreadable at 68px width
```

---

## Summary for Codex

Expect the animation source package to contain reusable 1-bit PNG assets plus manifests.

The end goal is not:

```text
one folder for every final ACT × LAYER × BATTERY × CHARGING × SIDE combination
```

The end goal is:

```text
a compact reusable asset library
+ scene manifests
+ firmware conversion scripts
+ C asset registry
+ generic compositor
+ synchronized dual-display scene planner
```

The “Voyager” story should guide visual art direction, but the firmware architecture should remain generic and reusable.
