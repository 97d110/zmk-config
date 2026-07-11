# Repo Map

- `boards/arm/eyelash_sofle/`
  Board module for the Eyelash Sofle. This is the source of truth for side definitions, DTS wiring, defconfigs, physical layout metadata, and board metadata.
- `config/`
  User-config layer. Holds the pinned `west.yml`, keyboard runtime config, keymap, and JSON layout metadata used by ZMK tooling.
- `build.yaml`
  Canonical build matrix. Includes normal left/right nice!view builds, a Studio build, settings-reset builds for both halves, a symmetric left/right USB-logging debug pair that matches the upstream display path, a separate left Studio debug build, symmetric display-engine debug builds for hardware investigation, and a left Studio display-engine debug build for Studio-specific isolation.
- `.github/workflows/build.yml`
  GitHub Actions entrypoint. Kept pinned to the ZMK `v0.3` workflow.
- `keymap_drawer.config.yaml`
  Source config for keymap-drawer generation.
- `keymap-drawer/`
  Generated output location for keymap diagrams and YAML exports.
- `scripts/agentic/verify.sh`
  Cheap structural validation invoked by `make verify`; `make verify-docker`
  runs the same script in a small Docker image with the required tools.
- `.agentic/context/display-engine-increment-0.md`
  Audit of the current nice!view display path, build/debug implications, and local insertion points for the dual-display scene engine.
- `.agentic/context/display-engine-logging-convention.md`
  Required logging policy for local display-engine code and future display increments.
- `.agentic/context/code-organization-convention.md`
  Required planning and coding boundary between durable product/core code and temporary/mock code, assets, fixtures, or scaffolding.
- `.agentic/context/display-engine-increment-1.md`
  Handoff note for the first local dual nice!view renderer slice.
- `.agentic/context/display-engine-increment-2.md`
  Handoff note for the shared state model, state-aware status planning, and
  placeholder status-value rendering.
- `.agentic/context/display-engine-increment-3.md`
  Handoff note for scene-kind selection, energy and charging modifiers,
  expanded animation plan, and the scene-aware mock renderer dispatch.
- `.agentic/context/display-engine-increment-4.md`
  Historical handoff note for the removed host-side simulator. Current
  simulator work uses the browser canvas app under `sim/`.
- `.agentic/context/display-engine-increment-5.md`
  Handoff note for the firmware state adapter, ZMK event subscriptions, and
  event-driven LVGL refresh path.
- `.agentic/context/display-engine-increment-5A.md`
  Handoff note for the lightweight configurable typing activity cycle,
  complete state logs, and semantic typing-to-idle lifecycle.
- `.agentic/context/display-engine-increment-5B.md`
  Handoff note for the boundary between core logical display state and future
  theme-specific animation state.
- `.agentic/context/display-engine-increment-6.md`
  Handoff note for the shared LVGL-free theme context, visible no-asset mock
  theme renderer, simulator ticks, and firmware theme refresh loop.
- `.agentic/context/display-engine-increment-7.md`
  Handoff note for the shared Timing Profile, generated C timing constants,
  simulator timing editor, scripted timing checks, visual display-sleep, and
  Core State / Display Plan / Animation State terminology alignment.
- `.agentic/context/display-engine-increment-8A.md`
  Handoff note for the v13 asset analysis, source-sprite interpretation, and
  planned shared render-recipe boundary.
- `.agentic/context/display-engine-increment-8B.md`
  Handoff note for the structural refactor: renamed the generic animation
  controller theme->animation and moved theme-specific content (assets, timing
  profile) to the root `themes/` tree.
- `.agentic/context/display-engine-increment-8C.md`
  Handoff note for the generic recipe command model (`display/render/recipe/`)
  and the space/v1 scene-recipe planner (`themes/space/v1/`), emitted as data in
  the sim and logged on firmware (not yet rendered).
- `.agentic/context/display-engine-increment-8D.md`
  Handoff note for the generic 1-bit compositor + asset-source interface, the
  space/v1 mock asset backend, and rendering the recipe on firmware and in the
  sim; retires the `display/mock/` placeholder renderer.
- `display/core/`
  Durable LVGL-free display state, mapping, planning types, and policy. The
  display contract is portrait: top/bottom edges are short, left/right edges
  are long.
- `display/render/lvgl/`
  Durable LVGL firmware adapter boundary, renderer contract, and viewport
  mapping. The contract is implemented by `display/render/lvgl/screen_renderer.c`,
  which composites the active theme's recipe into the animation region.
- `display/render/animation/`
  Durable generic, theme-independent animation controller shared by firmware and
  simulator builds. Owns renderer-local frame ticks, typing phase, decay state,
  visual display-sleep, and the animation snapshot. Reads a theme-supplied Timing
  Profile generated into C constants during firmware and simulator builds.
- `display/render/recipe/`
  Durable generic, theme-independent composition layer: the recipe command model
  and the 1-bit compositor. Assets are referenced by opaque integer ID; the
  theme-specific planner that emits recipe commands lives under `themes/`.
- `display/firmware/`
  Durable ZMK firmware state adapter. It maps runtime battery, activity,
  keypress, endpoint, layer, USB, BLE, and split-link sources into
  `display/core/` state and queues refreshes on the ZMK display work queue.
  It also runs the bounded animation refresh loop while renderer-local state
  wants another frame.
- `themes/`
  Root home for theme-specific content, versioned per theme (e.g.
  `themes/space/v1/`). Holds each theme's scene-recipe planner, asset-ID
  vocabulary, `timing_profile.json` tuning, source assets under `assets/`, and
  any temporary mock asset backend. No generic engine code lives here.
- `sim/`
  Browser canvas simulator for the dual-display scene engine. It provides
  a local Python serial bridge for real debug firmware logs, a host C runner
  under `sim/engine/` built from `display/core/` and `display/render/animation/`,
  a timing editor that updates the host engine's Timing Profile, scripted
  timing checks, and a canvas renderer in `sim/web/`. Core keyboard events from
  hardware are the controller; firmware display logs remain diagnostics only.
- `.agentic/troubleshooting/split-pairing.md`
  Short recovery note for stale BLE split bonds, including the requirement to reset both halves.
- `docs/display-firmware-animation-flow.md`
  Durable technical explanation of the firmware animation-control path,
  including ZMK event entry points, state lifecycle, configurable typing
  activity cycle, render lifecycle, and a complete system wiring diagram.
- `docs/display-render-recipe-spec.md`
  Durable specification for the shared render-recipe boundary and initial central
  typing-intensity composition behavior.
- `docs/.meta/zmk_dual_display_animation_theme_tech_spec_v5.md`
  Current v5 animation-theme planning spec. It summarizes implemented
  display-engine increments, preserves the lean core activity model, defines
  theme-local typing phases, lays out deployable future increments, and
  documents the animation asset organization contract.

## Ownership Boundaries

- Board-side hardware changes belong under `boards/arm/eyelash_sofle/`.
- Runtime and keymap behavior belong under `config/`.
- Build matrix and artifact naming belong in `build.yaml`.
- Debug logging policy belongs in `build.yaml` and `.agentic/commands.md`.
- Future local display-engine work should stay local to this repo and must not reintroduce donor repos as runtime dependencies.
- Display-engine code changes must follow `.agentic/context/display-engine-logging-convention.md`.
- Planning and code changes must follow `.agentic/context/code-organization-convention.md`.
- The display-engine boundary is `display/core/`, `display/firmware/`, `display/render/lvgl/`, `display/render/animation/`, `display/render/recipe/`, and `sim/` (all generic), plus theme-specific content under root `themes/<name>/<version>/`; see `context/display-engine-increment-0.md` before changing display wiring.
