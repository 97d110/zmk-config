# Tech Spec Request v3: Dual nice!view Scene Engine for `zmk-config`

You are acting as the technical lead for a display architecture change inside my working ZMK repository.

This repo is the real source of truth. It already builds and runs correctly on my keyboard, and any display-engine work must be planned and implemented here.

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

Design and implement a generic, reusable, state-driven dual-display scene engine for the two `nice_view` displays on my split keyboard.

The engine must:

- support both halves
- use a shared state model
- support side-aware composition
- use placeholder assets first
- support a desktop simulator before heavy firmware iteration
- be maintainable enough that I can later swap in final art without rewriting the logic

## Non-Goals

Do **not**:

- start with final meteor art
- depend on `mario-peripheral-animation`, `nice-view-gem`, or `zmk-nice-oled` as runtime modules
- replace the working board/config structure with a giant rewrite
- break existing normal, Studio, settings-reset, or debug artifact flows

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
- status-bar planning
- animation-region planning

LVGL should be a renderer adapter that consumes the core's plan.

### 3. Scene planning, not single-pack lookup

Do not reduce the system to one selector like:

```c
const struct anim_pack *select_anim_pack(const anim_state_t *state);
```

That is too narrow for the behavior I want.

Prefer a plan-based model:

- gather keyboard/display state
- select a base scene
- derive left/right screen plans
- fill a fixed top status bar
- fill a lower animation region
- let the renderer draw the result

## Display Contract

Both displays should follow this fixed structure:

```c
/* STATUS BAR */
----------------
/* ANIMATION  */
/* ANIMATION  */
```

This means:

- top band reserved for status
- lower area reserved for animation or scene art
- the animation region should remain visually dominant
- side differences should be intentional
- the system is not a free-form overlay canvas

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
- side-specific contents are allowed
- avoid wasting animation space on labels

## State Model

Create a generic state model that can express at least:

- side / role
- current layer bucket or mode
- typing activity / WPM bucket
- battery bucket
- charging state
- USB / BLE / disconnected transport state if relevant
- split-link state for the other half
- display idle / sleep state

Centralize these mappings in one place:

- layer -> mode
- WPM -> activity bucket
- battery percentage -> battery bucket

The exact names can change, but the model should resemble:

```c
typedef struct {
    bool is_left;
    bool other_half_connected;
    bool usb_active;
    bool ble_active;
    bool charging;
    bool sleeping;
    uint8_t battery_bucket;
    uint8_t layer_mode;
    uint8_t activity_bucket;
} display_state_t;
```

## Core Planning Model

Prefer a plan model closer to this:

```c
typedef struct {
    /* top-bar icon or slot decisions */
} status_bar_plan_t;

typedef struct {
    /* base scene and animation selection */
} animation_plan_t;

typedef struct {
    status_bar_plan_t top_bar;
    animation_plan_t animation;
} screen_plan_t;
```

```c
typedef struct {
    screen_plan_t left;
    screen_plan_t right;
} dual_screen_plan_t;
```

```c
void build_dual_screen_plan(const display_state_t *state,
                            dual_screen_plan_t *out_plan);
```

LVGL should render `dual_screen_plan_t`. It should not own the business logic.

## Repo Placement

Keep the implementation modular and local.

Suggested structure:

- `display/core/`
  - state types
  - mapping logic
  - scene selection
  - side policy
  - plan building
- `display/render/lvgl/`
  - LVGL adapters
  - top-bar rendering
  - animation playback
  - object lifecycle
- `display/assets/`
  - placeholder frame sets
  - asset registry
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

It should exercise:

- layer changes
- WPM/activity bucket changes
- charging
- battery bucket
- split-link state
- USB/BLE/disconnected state if represented
- sleep/idle state

## Increment Strategy

Proceed in small, reversible increments.

### Increment 0

- inspect current repo wiring
- identify the exact local insertion points
- verify how the existing `nice_view` path is reached
- confirm build-contract and debug-artifact implications

### Increment 1

- create a minimal local dual-screen abstraction
- render a placeholder top bar and placeholder animation region
- keep behavior simple and reversible

### Increment 2

- add the shared state model
- add centralized mapping logic
- add lightweight state-transition debug support

### Increment 3

- add scene planning and placeholder animations
- switch visuals by mode and activity bucket

### Increment 4

- add the Ubuntu simulator
- verify manual state switching for both screens

### Increment 5

- wire the real ZMK state/event sources
- make firmware behavior match simulator behavior

### Increment 6

- refine per-side top-bar responsibilities
- refine timing and placeholder animation behavior

### Increment 7

- only after the generic engine is proven, define the final meteor-theme asset pipeline

## Constraints

- prefer simple, readable C
- prefer modular code over giant files
- preserve the current build contract unless a change is intentional and documented
- keep Studio and debug workflows intact
- keep redraw/update cost in mind
- design for low-power monochrome displays
- do placeholder assets first
- do not fork simulator logic away from firmware logic

## Expected Working Style

When helping implement this:

1. inspect the repo before proposing major changes
2. stay anchored to `zmk-config`, not the older repo
3. keep each increment small and testable
4. prefer focused patches and small files
5. say exactly what to build or run after each increment
6. call out any impact on `build.yaml`, `.agentic/commands.md`, or debug artifacts

## Default Assumptions

Unless I override them, assume:

- both halves remain `nice_view` targets
- this repo remains pinned to ZMK `v0.3`
- donor repos stay out of `config/west.yml`
- the status bar is icon-oriented
- the lower area is the primary animation region
- final art comes only after the generic engine is proven
