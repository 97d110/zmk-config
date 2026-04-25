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

- Generate anything related to the final art.
- Do not use art style related variable names, keep with logic and component related naming conventions.
- depend on `mario-peripheral-animation`, `nice-view-gem`, or `zmk-nice-oled` as runtime modules.
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
- Time typing length (For how long I'm typing on the keyboard.) and idle sleep states -> typing activity bucket
- battery percentage and charging status and state -> battery bucket

The exact names can change, but the model should resemble:

```c
typedef struct {
    bool is_left;
    bool other_half_connected;
    bool usb_active;
    bool ble_active;
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
  - durable asset registry
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
- display the dual-display scene-engine logs while simulator state changes and rendering decisions occur

It should exercise the same state structure mentioned earlier for the core logic.

## Increment Strategy

Proceed in small, reversible increments.

### Logging convention for increments 1-7

- All display-engine code changes must follow `.agentic/context/display-engine-logging-convention.md`.
- Add debug-level diagnostics for new state mapping, planning, rendering, simulator, firmware adapter, and timing logic.
- Keep normal firmware artifacts free of debug logging snippets or elevated log settings; use the dedicated debug artifacts to view runtime logs.
- Update the logging convention in the same change if a future subsystem needs different logging behavior.

### Code organization convention for increments 1-7

- All planning and code changes must follow `.agentic/context/code-organization-convention.md`.
- Plans must explicitly distinguish durable engine logic/assets from temporary mock logic/assets.
- Keep `display/core/` generic and free of mock-only naming, placeholder asset IDs, final art style names, and LVGL object details.
- Keep throwaway placeholders, mock icon geometry, and proof-of-concept asset logic under `display/mock/` so they can be replaced or deleted as a unit.

### Increment 0

- inspect current repo wiring
- identify the exact local insertion points
- verify how the existing `nice_view` path is reached
- confirm build-contract and debug-artifact implications

Increment 0 audit result:

- No engine code was added and no release-contract files were changed during the audit.
- `build.yaml` remains the release contract:
  - normal left and right artifacts use `shield: nice_view`
  - the Studio artifact uses `shield: nice_view` plus `studio-rpc-usb-uart`
  - settings-reset artifacts use `shield: settings_reset`
  - debug artifacts are dedicated `nice_view` builds with explicit USB logging snippets/CMake args
- `config/west.yml` remains minimal and pinned to ZMK `v0.3`; donor display repos are not present and must not be added as runtime dependencies.
- This repo is made available to Zephyr as a local board root through `zephyr/module.yml` with `build.settings.board_root: .`.
- Both halves enable ZMK display support in their board defconfigs with `CONFIG_ZMK_DISPLAY=y`.
- The left half is the split central through `CONFIG_ZMK_SPLIT_ROLE_CENTRAL default y` under `BOARD_EYELASH_SOFLE_LEFT`.
- The local board DTS provides the upstream nice!view-required SPI node at `nice_view_spi: &spi0`, with SCK `P0.20`, MOSI `P0.17`, MISO `P0.25`, and CS `P0.6`.
- The upstream ZMK `nice_view` shield overlay binds a Sharp LS0xx display at `160x68` and sets `zephyr,display = &nice_view`.
- The installed Eyelash Sofle displays are physically portrait/vertical
  rectangles. The local engine must plan them as short top/bottom edge by long
  left/right edge, regardless of the upstream shield binding wording.
- ZMK display init calls `zmk_display_status_screen()` and loads the returned LVGL screen.
- Upstream `nice_view` currently provides a strong `zmk_display_status_screen()` via `custom_status_screen.c` when `CONFIG_NICE_VIEW_WIDGET_STATUS` is enabled.
- A local LVGL renderer must avoid duplicate `zmk_display_status_screen()` definitions. The clean path is to disable the upstream `NICE_VIEW_WIDGET_STATUS` for scene-engine builds, then compile the local renderer as the status screen provider.
- New firmware display sources must be guarded so `settings_reset` builds do not pick up the engine.
- Normal builds must remain free of USB logging; debug logging stays confined to the dedicated debug artifacts.
- The audit details are recorded for future agents in `.agentic/context/display-engine-increment-0.md`.
- `make verify` passed during increment 0. A CMake-only west configure was attempted, but `west` was not available on the shell `PATH`.

### Increment 1

- create a minimal local dual-screen abstraction
- render a placeholder top bar and placeholder animation region
- placeholder geometry must read top-to-bottom on a portrait display:
  short-edge status bar first, lower animation region second
- side-specific placeholder variation must not make the display look like a left/right panel split
- keep all mock placeholder drawing and throwaway asset logic under `display/mock/` so it can be replaced or deleted as a unit
- keep `display/core/` free of mock asset names, LVGL objects, and temporary rendering details
- keep behavior simple and reversible
- establish the display-engine logging convention and add debug-level trace points in the new planning/rendering logic

### Increment 2

- add the shared state model
- add centralized mapping logic
- add lightweight state-transition debug support
- follow the logging convention for state capture, mapping decisions, and recoverable fallbacks

### Increment 3

- add scene planning and placeholder animations
- switch visuals by mode and activity bucket
- follow the logging convention for scene selection and animation placeholder changes

### Increment 4

- add the Ubuntu simulator
- verify manual state switching for both screens
- make simulator diagnostics consistent with the logging convention without forking core logic
- show the `zmk_dual_display` scene-engine logs in the simulator UI or adjacent simulator console output

### Increment 5

- wire the real ZMK state/event sources
- make firmware behavior match simulator behavior
- follow the logging convention for ZMK event adapters, state updates, and ignored/no-op events

### Increment 6

- refine per-side top-bar responsibilities
- refine timing and placeholder animation behavior
- follow the logging convention for timing decisions while avoiding unconditional per-frame hot-path logs

### Increment 7

- only after the generic engine is proven, define the final meteor-theme asset pipeline
- follow the logging convention for asset registry/pipeline decisions without using final-art-specific logic names

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
