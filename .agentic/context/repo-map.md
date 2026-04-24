# Repo Map

- `boards/arm/eyelash_sofle/`
  Board module for the Eyelash Sofle. This is the source of truth for side definitions, DTS wiring, defconfigs, physical layout metadata, and board metadata.
- `config/`
  User-config layer. Holds the pinned `west.yml`, keyboard runtime config, keymap, and JSON layout metadata used by ZMK tooling.
- `build.yaml`
  Canonical build matrix. Includes normal left/right nice!view builds, a Studio build, a settings-reset build, and left/right USB-logging debug builds.
- `.github/workflows/build.yml`
  GitHub Actions entrypoint. Kept pinned to the ZMK `v0.3` workflow.
- `keymap_drawer.config.yaml`
  Source config for keymap-drawer generation.
- `keymap-drawer/`
  Generated output location for keymap diagrams and YAML exports.
- `scripts/agentic/verify.sh`
  Cheap structural validation invoked by `make verify`.

## Ownership Boundaries

- Board-side hardware changes belong under `boards/arm/eyelash_sofle/`.
- Runtime and keymap behavior belong under `config/`.
- Build matrix and artifact naming belong in `build.yaml`.
- Debug logging policy belongs in `build.yaml` and `.agentic/commands.md`.
- Future local display-engine work should stay local to this repo and must not reintroduce donor repos as runtime dependencies.
