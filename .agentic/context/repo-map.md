# Repo Map

- `boards/arm/eyelash_sofle/`
  Board module for the Eyelash Sofle. This is the source of truth for side definitions, DTS wiring, defconfigs, physical layout metadata, and board metadata.
- `config/`
  User-config layer. Holds the pinned `west.yml`, keyboard runtime config, keymap, and JSON layout metadata used by ZMK tooling.
- `build.yaml`
  Canonical build matrix. Includes normal left/right nice!view builds, a Studio build, settings-reset builds for both halves, and left/right USB-logging debug builds.
- `.github/workflows/build.yml`
  GitHub Actions entrypoint. Kept pinned to the ZMK `v0.3` workflow.
- `keymap_drawer.config.yaml`
  Source config for keymap-drawer generation.
- `keymap-drawer/`
  Generated output location for keymap diagrams and YAML exports.
- `scripts/agentic/verify.sh`
  Cheap structural validation invoked by `make verify`.
- `.agentic/context/display-engine-increment-0.md`
  Audit of the current nice!view display path, build/debug implications, and local insertion points for the dual-display scene engine.
- `.agentic/context/display-engine-logging-convention.md`
  Required logging policy for local display-engine code and future display increments.
- `.agentic/context/display-engine-increment-1.md`
  Handoff note for the first local dual nice!view renderer slice.
- `.agentic/troubleshooting/split-pairing.md`
  Short recovery note for stale BLE split bonds, including the requirement to reset both halves.

## Ownership Boundaries

- Board-side hardware changes belong under `boards/arm/eyelash_sofle/`.
- Runtime and keymap behavior belong under `config/`.
- Build matrix and artifact naming belong in `build.yaml`.
- Debug logging policy belongs in `build.yaml` and `.agentic/commands.md`.
- Future local display-engine work should stay local to this repo and must not reintroduce donor repos as runtime dependencies.
- Display-engine code changes must follow `.agentic/context/display-engine-logging-convention.md`.
- The planned display-engine boundary is `display/core/`, `display/firmware/`, `display/render/lvgl/`, `display/assets/`, and `sim/`; see `context/display-engine-increment-0.md` before changing display wiring.
