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
  Handoff note for the Ubuntu console simulator, manual state switching, and
  host-side display-engine logging.
- `.agentic/context/display-engine-increment-5.md`
  Handoff note for the firmware state adapter, ZMK event subscriptions, and
  event-driven LVGL refresh path.
- `.agentic/context/display-engine-increment-5A.md`
  Handoff note for the lightweight configurable typing activity cycle,
  complete state logs, and semantic typing-to-idle lifecycle.
- `.agentic/context/display-engine-increment-5B.md`
  Handoff note for the boundary between core logical display state and future
  theme-specific animation state.
- `display/core/`
  Durable LVGL-free display state, mapping, planning types, and policy. The
  display contract is portrait: top/bottom edges are short, left/right edges
  are long.
- `display/render/lvgl/`
  Durable LVGL firmware adapter boundary, renderer contract, and viewport
  mapping. The current renderer contract is implemented by `display/mock/`.
- `display/firmware/`
  Durable ZMK firmware state adapter. It maps runtime battery, activity,
  keypress, endpoint, layer, USB, BLE, and split-link sources into
  `display/core/` state and queues refreshes on the ZMK display work queue.
- `display/mock/`
  Temporary proof-of-concept placeholder rendering. It must preserve the
  portrait display contract and should be easy to replace or delete.
- `sim/`
  Ubuntu console and browser simulator for the dual-display scene engine. It
  compiles the durable `display/core/` sources directly, provides manual state
  controls, and renders both screen plans as a compact preview with adjacent
  `zmk_dual_display` logs. The Dockerized web app lives in `sim/web/`.
- `.agentic/troubleshooting/split-pairing.md`
  Short recovery note for stale BLE split bonds, including the requirement to reset both halves.
- `docs/display-firmware-animation-flow.md`
  Durable technical explanation of the firmware animation-control path,
  including ZMK event entry points, state lifecycle, configurable typing
  activity cycle, render lifecycle, and a complete system wiring diagram.

## Ownership Boundaries

- Board-side hardware changes belong under `boards/arm/eyelash_sofle/`.
- Runtime and keymap behavior belong under `config/`.
- Build matrix and artifact naming belong in `build.yaml`.
- Debug logging policy belongs in `build.yaml` and `.agentic/commands.md`.
- Future local display-engine work should stay local to this repo and must not reintroduce donor repos as runtime dependencies.
- Display-engine code changes must follow `.agentic/context/display-engine-logging-convention.md`.
- Planning and code changes must follow `.agentic/context/code-organization-convention.md`.
- The planned display-engine boundary is `display/core/`, `display/firmware/`, `display/render/lvgl/`, `display/assets/`, and `sim/`; see `context/display-engine-increment-0.md` before changing display wiring.
