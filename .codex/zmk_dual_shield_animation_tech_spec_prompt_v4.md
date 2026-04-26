# Tech Spec Request v4: Dual nice!view Scene Engine for `zmk-config`

You are acting as the technical lead for a display architecture change inside my
working ZMK repository.

This repo is the real source of truth. It already builds and runs correctly on
my keyboard, and any display-engine work must be planned and implemented here.

This v4 spec supersedes v3 by preserving the original increment order while
capturing the useful takeaways from the separate animation state requirements
review. In particular, v4 makes scene selection, modifier composition, fallback
behavior, asset strategy, and dual-display timing more explicit without pulling
final art names into durable engine APIs.

## Repository Context

This repository already contains:

- the Eyelash Sofle board module
- the user config used by CI and local builds
- both left and right `nice_view` firmware builds in `build.yaml`
- dedicated Studio, settings-reset, and USB-logging debug artifacts
- a pinned ZMK `v0.3` workspace model in `config/west.yml`
- a local `.zmk/` workspace for builds

Important repo rules:

- `build.yaml` is the release contract
- `config/west.yml` must stay minimal and pinned to `v0.3`
- do **not** add donor display repos as runtime dependencies
- future display-engine work must stay local to this repo
- normal builds should remain free of debug logging snippets

## Goal

Design and implement a generic, reusable, state-driven dual-display scene engine
for the two `nice_view` displays on my split keyboard.

The engine must:

- support both halves
- use a shared state model
- support side-aware and role-aware composition
- use placeholder assets first
- support a desktop simulator before heavy firmware iteration
- be maintainable enough that I can later swap in final art without rewriting
  the logic
- avoid storing every possible state combination as unique full-screen assets
- keep the final visual story represented as data and assets consumed by the
  engine, not as hardcoded renderer logic

## Non-Goals

Do **not**:

- generate anything related to the final art before the generic engine is proven
- use final-art-specific names in durable engine APIs or `display/core/`
- depend on `mario-peripheral-animation`, `nice-view-gem`, or `zmk-nice-oled`
  as runtime modules
- replace the working board/config structure with a giant rewrite
- break existing normal, Studio, settings-reset, or debug artifact flows
- decode PNGs at runtime on the keyboard
- make the status bar part of every animation frame asset

Theme-specific words and concepts may appear later in:

- future asset metadata
- generated asset registries
- theme recipe data
- mock/demo notes
- final art pipeline documentation

They should not appear in durable public planning APIs unless they are first
generalized into behavior-oriented terms.

## Architecture Direction

### 1. Local ownership

Keep the implementation local to this repo.

External repos are allowed only as:

- adjacent VSCode workspace references
- examples to inspect
- optional donor sources for ideas

They are **not** runtime dependencies.

### 2. Pure logic core

Do not make LVGL the center of the design.

The core should own:

- shared state types
- state mapping
- scene selection
- per-side planning
- role-aware planning
- status-bar planning
- animation-region planning
- scene modifier composition
- safe fallback selection

LVGL should be a renderer adapter that consumes the core's plan.

### 3. Scene planning, not single-pack lookup

Do not reduce the system to one selector like:

```c
const struct anim_pack *select_anim_pack(const anim_state_t *state);
```

That is too narrow for the behavior I want.

Prefer a plan-based model:

- gather keyboard/display state
- normalize it into the shared display state
- select a base scene kind
- apply layer, battery, charging, and role modifiers
- derive left/right screen plans
- fill a fixed top status bar
- fill a lower animation region
- let the renderer draw the result

The renderer should not need to know what a final theme concept means.

## Display Contract

Both displays should follow this fixed structure:

```c
/* STATUS BAR */
----------------
/* ANIMATION  */
/* ANIMATION  */
```

This means:

- the physical display is a vertical/portrait rectangle
- the top and bottom edges are the short edges
- the left and right edges are the long edges
- local engine coordinates should treat the short top edge as display width and
  the long side edge as display height
- top band reserved for status
- the top status bar is spread across the narrow top edge
- lower area reserved for animation or scene art
- the animation region should remain visually dominant
- side differences should be intentional
- the system is not a free-form overlay canvas
- the animation region must not overwrite the status bar unless a future
  full-screen scene mode is explicitly added

### Left display status bar priority

The left display should prioritize stability-related values:

- battery
- other shield connectivity
- USB / BT connectivity
- other connection or stability indicators if useful

### Right display status bar priority

The right display should prioritize functionality-related values:

- battery
- other shield connectivity
- current layer
- other active-mode indicators if useful

### Shared status-bar rules

- icon-first, not text-heavy
- consistent geometry on both sides
- fit the narrow portrait top edge; do not assume landscape status-bar width
- side-specific contents are allowed
- avoid wasting animation space on labels
- status planning stays separate from animation scene planning

## State Model

Create a generic state model that can express at least:

- side
- role
- current layer bucket or mode
- typing activity bucket
- battery bucket
- USB / BLE / disconnected transport state if relevant
- split-link state for the other half

Centralize these mappings in one place:

- layer -> mode
- typing streak length and idle/sleep state -> typing activity bucket
- battery percentage and charging status -> battery bucket
- transport runtime flags -> transport state
- split runtime flags -> split-link state

The exact names can change, but the durable model should resemble:

```c
struct zmk_dual_display_state {
    enum zmk_dual_display_side side;
    enum zmk_dual_display_role role;
    enum zmk_dual_display_battery_bucket battery;
    enum zmk_dual_display_activity_bucket activity;
    enum zmk_dual_display_transport_state transport;
    enum zmk_dual_display_split_link_state split_link;
    enum zmk_dual_display_layer_mode layer;
};
```

### Side and role

Both halves must be supported.

Do not assume long-term that left always means the primary visual role and right
always means the secondary visual role. The current implementation may map
primary/secondary by side, but the design should remain flexible enough to
support role-based mapping later.

### Activity buckets

Activity buckets should express user activity, not folders of full-frame images:

- sleep
- idle
- typing 2s
- typing 5s
- typing 10s
- typing 15s

Sleep is an override. Typing buckets should drive motion intensity and scene
energy, not force unique asset sets for every other state dimension.

### Layer modes

Layer mode should usually modify a base scene rather than replace every asset.
Keep the durable names generic and keyboard-oriented:

- unknown
- type
- symbol
- mod
- config

Theme-specific interpretations belong later in theme recipe data.

### Battery buckets

Battery buckets should support:

- unknown
- 0-10
- 11-50
- 51-100
- each known bucket with charging state

Battery should not force unique full-frame sequences. Prefer generic modifiers
such as low/medium/high energy, scale choices, dither intensity, alternate
sprites, or overlay enablement.

### Split-link state

Split-link disconnected state should override normal scene selection. It may
still allow side or role variation inside the error/fallback scene.

Unknown split-link state should render a safe warning/fallback, not crash or
hide the rest of the display engine.

### Transport state

Transport is primarily status-bar information. Do not let USB/BT/disconnected
transport multiply the animation asset matrix.

## Scene Selection Priority

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

Example behavior:

```text
If sleeping:
  render a sleep/blank/minimal scene regardless of layer or battery.

If split disconnected:
  render an error/fallback scene regardless of activity,
  while still allowing side/role variation.

If typing for 10s on the symbol layer while charging:
  select the high-activity base scene
  apply the symbol layer flavor
  apply the battery intensity modifier
  enable the charging overlay
  choose the side/role variant
```

The order above is a planning rule. It should be captured in durable core code
with debug-level diagnostics for scene decisions and warning-level diagnostics
for invalid or unknown enum fallbacks.

## Core Planning Model

Prefer a plan model close to this:

```c
struct zmk_dual_display_status_bar_plan {
    /* top-bar icon or slot decisions */
};

struct zmk_dual_display_animation_plan {
    /* animation-region scene and modifier decisions */
};

struct zmk_dual_display_screen_plan {
    enum zmk_dual_display_side side;
    struct zmk_dual_display_status_bar_plan status_bar;
    struct zmk_dual_display_animation_plan animation;
};

struct zmk_dual_display_dual_plan {
    struct zmk_dual_display_screen_plan left;
    struct zmk_dual_display_screen_plan right;
};
```

LVGL should render screen plans. It should not own business logic.

The animation plan should evolve toward generic, behavior-oriented fields such
as:

```c
enum zmk_dual_display_scene_kind {
    ZMK_DUAL_DISPLAY_SCENE_NORMAL,
    ZMK_DUAL_DISPLAY_SCENE_SLEEP,
    ZMK_DUAL_DISPLAY_SCENE_LINK_ERROR,
    ZMK_DUAL_DISPLAY_SCENE_FALLBACK,
};

enum zmk_dual_display_scene_variant {
    ZMK_DUAL_DISPLAY_SCENE_VARIANT_PRIMARY,
    ZMK_DUAL_DISPLAY_SCENE_VARIANT_SECONDARY,
};

enum zmk_dual_display_energy_level {
    ZMK_DUAL_DISPLAY_ENERGY_UNKNOWN,
    ZMK_DUAL_DISPLAY_ENERGY_LOW,
    ZMK_DUAL_DISPLAY_ENERGY_MEDIUM,
    ZMK_DUAL_DISPLAY_ENERGY_HIGH,
};

struct zmk_dual_display_animation_plan {
    struct zmk_dual_display_rect bounds;
    enum zmk_dual_display_scene_kind scene;
    enum zmk_dual_display_scene_variant variant;
    enum zmk_dual_display_activity_bucket activity;
    enum zmk_dual_display_layer_mode layer;
    enum zmk_dual_display_energy_level energy;
    bool charging_overlay;
};
```

These are conceptual field examples. Exact names can change if the repo's
existing code suggests a cleaner local convention.

Do not put final-theme asset IDs or final-art object names into this durable
plan. If a theme needs them later, map generic plan fields to theme recipe data
outside the durable core planning API.

## Avoid Full Combination Explosion

Do not design around this:

```text
[ACTIVITY][LAYER][BATTERY][CHARGING][SIDE] -> unique folder of full-frame images
```

Design around this instead:

```text
state -> generic scene plan -> optional scene recipe -> reusable draw commands
```

A future recipe may combine:

- background class
- primary object class
- effect class
- motion profile
- procedural effect selection
- battery/energy modifier
- charging overlay flag
- role/side variant

Keep status-bar contents separate from animation recipes.

## Renderer And Compositor Direction

The durable renderer boundary should be generic.

A lower-level draw plan may eventually use something like:

```c
struct zmk_dual_display_draw_command {
    enum zmk_dual_display_asset_id asset;
    int16_t x;
    int16_t y;
    uint8_t frame;
    enum zmk_dual_display_blend_mode blend;
};
```

The renderer/compositor should know how to:

- clear a framebuffer or region
- draw a bitmap/sprite
- apply a mask
- draw or apply simple procedural effects
- compose overlays
- flush a frame

The renderer/compositor should not need to know what a final theme concept,
layer story, or battery story means. Those meanings should already have been
resolved by the planner and theme recipe data.

## Asset Strategy

Final firmware should use 1-bit assets and should not rely on runtime PNG
decoding.

Expected later pipeline:

```text
source PNGs
  -> conversion script
  -> 1-bit packed bitmap/sprite data
  -> C asset registry
  -> runtime renderer/compositor
```

Final firmware asset constraints:

- black: `#000000`
- white: `#FFFFFF`
- no gray
- no runtime PNG alpha
- transparency represented by masks
- missing assets must render a placeholder/error glyph instead of crashing

Recommended durable asset struct concept:

```c
struct zmk_dual_display_bitmap_asset {
    const uint8_t *pixels;
    const uint8_t *mask;
    uint8_t width;
    uint8_t height;
    int8_t anchor_x;
    int8_t anchor_y;
};
```

Recommended generic blend modes:

- copy with mask
- OR white pixels
- clear black pixels
- XOR mask
- invert region

Do not build the final art pipeline before the generic engine, simulator, timing
model, and placeholder scene composition are proven.

## Timing And Synchronization

Both displays should use compatible logical timing.

Requirements:

- side-specific scenes may differ visually but should stay synchronized
- animation loops should prefer compatible lengths such as 12, 18, or 24 frames
- background/procedural effect speed should be derived from the same activity
  plan for both halves
- animation should support seamless loops
- display rendering must not block typing or BLE behavior
- per-frame logs must be gated or disabled by default

Do not assume both halves can share a literal real-time clock in all firmware
conditions. A practical implementation should derive frame indices from local
uptime plus a state epoch or equivalent stable timing input. Later firmware work
can refine whether the central half needs to publish timing hints to the
peripheral half.

## Required Fallbacks

Every state dimension needs safe fallback behavior.

Required fallbacks:

- unknown side -> normalized side or safe primary variant
- unknown role -> safe side-based variant
- unknown battery -> unknown battery icon plus neutral animation modifier
- unknown layer -> neutral/default visual modifier
- unknown transport -> unknown status icon
- unknown split link -> warning/fallback icon
- invalid scene enum -> fallback scene
- missing asset -> placeholder rectangle/checker/error glyph

No missing asset, invalid enum, or unknown runtime state should crash the display
engine.

## Repo Placement

Keep the implementation modular and local.

Suggested structure:

- `display/core/`
  - state types
  - mapping logic
  - scene selection
  - modifier composition
  - side/role policy
  - plan building
- `display/render/lvgl/`
  - LVGL adapters
  - top-bar rendering
  - animation playback
  - object lifecycle
  - durable renderer contract
  - viewport mapping
- `display/assets/`
  - durable asset registry
  - generated C asset tables
  - long-lived frame sets after the generic engine is proven
- `display/mock/`
  - temporary placeholder rendering
  - mock icon/scene geometry
  - throwaway proof-of-concept asset logic
- `display/firmware/`
  - ZMK-facing state adapters
  - event listeners
  - debug logging helpers
- `sim/`
  - desktop harness
  - Linux build files
  - state controls

Exact names can change, but keep the boundaries clean.

## Reference Usage

Use adjacent repos for reference only.

Primary reference intent:

- `mario-peripheral-animation`
  - animation/state ideas
- `nice-view-gem`
  - display/shield structure ideas
- `zmk-nice-oled`
  - secondary display composition ideas

Do not turn these into runtime dependencies or add them to `config/west.yml`.

## Simulator Requirement

The simulator is a first-class requirement.

It should:

- run on Ubuntu
- preview both displays without flashing firmware
- share the same core logic as firmware
- allow manual state changes
- validate the fixed top-bar plus animation-region contract
- exercise the same state structure as firmware
- display the `zmk_dual_display` scene-engine logs in the simulator UI or
  adjacent simulator console output

Do not fork simulator logic away from firmware logic.

## Increment Strategy

Proceed in small, reversible increments.

### Logging convention for increments 1+

- All display-engine code changes must follow
  `.agentic/context/display-engine-logging-convention.md`.
- Add debug-level diagnostics for new state mapping, planning, rendering,
  simulator, firmware adapter, timing, asset registry, and compositor logic.
- Keep normal firmware artifacts free of debug logging snippets or elevated log
  settings; use the dedicated debug artifacts to view runtime logs.
- Update the logging convention in the same change if a future subsystem needs
  different logging behavior.

### Code organization convention for increments 1+

- All planning and code changes must follow
  `.agentic/context/code-organization-convention.md`.
- Plans must explicitly distinguish durable engine logic/assets from temporary
  mock logic/assets.
- Keep `display/core/` generic and free of mock-only naming, placeholder asset
  IDs, final art style names, and LVGL object details.
- Keep throwaway placeholders, mock icon geometry, and proof-of-concept asset
  logic under `display/mock/` so they can be replaced or deleted as a unit.

### Increment 0

- inspect current repo wiring
- identify the exact local insertion points
- verify how the existing `nice_view` path is reached
- confirm build-contract and debug-artifact implications

Increment 0 audit result:

- No engine code was added and no release-contract files were changed during
  the audit.
- `build.yaml` remains the release contract:
  - normal left and right artifacts use `shield: nice_view`
  - the Studio artifact uses `shield: nice_view` plus `studio-rpc-usb-uart`
  - settings-reset artifacts use `shield: settings_reset`
  - debug artifacts are dedicated `nice_view` builds with explicit USB logging
    snippets/CMake args
- `config/west.yml` remains minimal and pinned to ZMK `v0.3`; donor display
  repos are not present and must not be added as runtime dependencies.
- This repo is made available to Zephyr as a local board root through
  `zephyr/module.yml` with `build.settings.board_root: .`.
- Both halves enable ZMK display support in their board defconfigs with
  `CONFIG_ZMK_DISPLAY=y`.
- The left half is the split central through `CONFIG_ZMK_SPLIT_ROLE_CENTRAL
  default y` under `BOARD_EYELASH_SOFLE_LEFT`.
- The local board DTS provides the upstream nice!view-required SPI node at
  `nice_view_spi: &spi0`, with SCK `P0.20`, MOSI `P0.17`, MISO `P0.25`, and CS
  `P0.6`.
- The upstream ZMK `nice_view` shield overlay binds a Sharp LS0xx display at
  `160x68` and sets `zephyr,display = &nice_view`.
- The installed Eyelash Sofle displays are physically portrait/vertical
  rectangles. The local engine must plan them as short top/bottom edge by long
  left/right edge, regardless of the upstream shield binding wording.
- ZMK display init calls `zmk_display_status_screen()` and loads the returned
  LVGL screen.
- Upstream `nice_view` currently provides a strong `zmk_display_status_screen()`
  via `custom_status_screen.c` when `CONFIG_NICE_VIEW_WIDGET_STATUS` is enabled.
- A local LVGL renderer must avoid duplicate `zmk_display_status_screen()`
  definitions. The clean path is to disable the upstream `NICE_VIEW_WIDGET_STATUS`
  for scene-engine builds, then compile the local renderer as the status screen
  provider.
- New firmware display sources must be guarded so `settings_reset` builds do not
  pick up the engine.
- Normal builds must remain free of USB logging; debug logging stays confined to
  the dedicated debug artifacts.
- The audit details are recorded for future agents in
  `.agentic/context/display-engine-increment-0.md`.
- `make verify` passed during increment 0. A CMake-only west configure was
  attempted, but `west` was not available on the shell `PATH`.

### Increment 1

- create a minimal local dual-screen abstraction
- render a placeholder top bar and placeholder animation region
- placeholder geometry must read top-to-bottom on a portrait display:
  short-edge status bar first, lower animation region second
- side-specific placeholder variation must not make the display look like a
  left/right panel split
- keep all mock placeholder drawing and throwaway asset logic under
  `display/mock/` so it can be replaced or deleted as a unit
- keep `display/core/` free of mock asset names, LVGL objects, and temporary
  rendering details
- keep behavior simple and reversible
- establish the display-engine logging convention and add debug-level trace
  points in the new planning/rendering logic

### Increment 2

- add the shared state model
- add centralized mapping logic
- add lightweight state-transition debug support
- follow the logging convention for state capture, mapping decisions, and
  recoverable fallbacks

### Increment 3

- add durable scene planning for the lower animation region
- preserve status-bar planning as a separate sibling plan
- add generic scene selection priority:
  - sleep override
  - split/link error override
  - activity bucket
  - layer mode modifier
  - battery/energy modifier
  - charging overlay flag
  - side/role variant
- extend the animation plan with generic fields only, such as scene kind, layer
  mode, energy level, charging overlay, and scene variant
- switch placeholder visuals by activity and layer mode without adding final art
  names to durable APIs
- keep battery and charging as modifiers, not separate full scene families
- keep transport as status-bar information only
- follow the logging convention for scene selection, modifier selection,
  fallback handling, and placeholder animation changes

### Increment 4

- add the Ubuntu simulator
- verify manual state switching for both screens
- make simulator diagnostics consistent with the logging convention without
  forking core logic
- show the `zmk_dual_display` scene-engine logs in the simulator UI or adjacent
  simulator console output
- use the simulator to exercise the scene priority rules added in Increment 3

### Increment 5

- wire the real ZMK state/event sources
- make firmware behavior match simulator behavior
- capture layer, typing activity, battery, transport, and split-link inputs from
  real ZMK sources where available
- preserve safe unknown/fallback behavior when a runtime source is unavailable
- follow the logging convention for ZMK event adapters, state updates, and
  ignored/no-op events

### Increment 6

- refine per-side top-bar responsibilities
- refine timing and placeholder animation behavior
- introduce a practical logical frame clock for both halves
- derive placeholder frame indices from stable local timing plus state epoch or
  equivalent state-transition timing
- keep loop lengths compatible across sides
- avoid unconditional per-frame hot-path logs
- follow the logging convention for timing decisions and state-gated animation
  changes

### Increment 7

Only after the generic engine, simulator, firmware state wiring, and placeholder
timing model are proven, define the durable generic animation asset/compositor
architecture.

Increment 7 should be specific, but still generic:

- add or document a durable asset registry interface using fake assets first
- define the generic bitmap asset shape, including pixel data, optional mask,
  dimensions, and anchors
- define generic blend modes:
  - copy with mask
  - OR white pixels
  - clear black pixels
  - XOR mask
  - invert region
- add a tiny compositor spike that can combine:
  - one background or fill command
  - one primary object sprite
  - one overlay/effect command
  - one missing-asset fallback glyph
- prove that battery/energy, charging, layer, and side/role modifiers can alter
  the draw plan without duplicating full-frame scene families
- keep final-theme names out of the durable renderer and core APIs
- keep placeholder/fake assets clearly isolated so they can be replaced or
  deleted later
- follow the logging convention for asset lookup, missing-asset fallback,
  compositor decisions, and render-plan construction

### Increment 8

Define the final theme asset pipeline and recipe boundary, without yet requiring
the complete final art set.

Increment 8 should capture:

- source PNG or source-art conventions
- conversion script expectations
- 1-bit packed bitmap output format
- generated C registry shape
- mask handling
- anchor handling
- naming rules for theme-specific assets
- where final-theme data may use final visual names
- where durable engine APIs must remain generic
- how limited pre-rendered sequences are allowed for complex visuals
- how recipe data maps generic scene plans to concrete assets

Allowed limited pre-rendered sequence categories should be documented only after
the compositor path is proven. Full-frame sequences should remain the exception,
not the default.

### Increment 9

Integrate a small first real or semi-real asset set through the pipeline.

This increment should prove:

- generated 1-bit assets build in firmware
- the simulator and firmware use the same asset registry shape
- missing-asset fallback still works
- a small set of reusable assets can create multiple visible states
- layer mode changes visual flavor without duplicating every base frame
- battery level changes visual intensity without requiring separate full-frame
  animations
- charging adds an overlay without duplicating the base scene
- primary/secondary or role variants can show different synchronized visuals

This is still not the complete final art pass.

### Increment 10

Do final visual recipe expansion and performance hardening.

This increment should:

- fill out the final theme recipes
- tune frame timing and loop lengths
- check redraw/update cost
- verify that display rendering does not block keyboard behavior
- check memory impact of assets and masks
- verify normal, Studio, settings-reset, and debug artifact flows still behave
  as intended
- update `.agentic/commands.md` and `.agentic/context/repo-map.md` if build or
  workflow assumptions changed

## Constraints

- prefer simple, readable C
- prefer modular code over giant files
- preserve the current build contract unless a change is intentional and
  documented
- keep Studio and debug workflows intact
- keep redraw/update cost in mind
- design for low-power monochrome displays
- do placeholder assets first
- do not fork simulator logic away from firmware logic
- do not let final visual story names leak into durable engine contracts
- do not let state dimensions multiply into full-frame asset folders

## Expected Working Style

When helping implement this:

1. inspect the repo before proposing major changes
2. stay anchored to `zmk-config`, not the older repo
3. keep each increment small and testable
4. prefer focused patches and small files
5. say exactly what to build or run after each increment
6. call out any impact on `build.yaml`, `.agentic/commands.md`, or debug
   artifacts
7. explicitly separate durable engine work from temporary/mock work
8. explain how later theme-specific data maps into generic engine concepts

## Default Assumptions

Unless I override them, assume:

- both halves remain `nice_view` targets
- this repo remains pinned to ZMK `v0.3`
- donor repos stay out of `config/west.yml`
- the status bar is icon-oriented
- the lower area is the primary animation region
- status-bar planning stays separate from animation planning
- final art comes only after the generic engine is proven
- role support should remain first-class even if early variants are side-derived
