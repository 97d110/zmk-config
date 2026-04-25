# AGENTS.md

This repository is the primary ZMK config and board-module home for the Eyelash Sofle split keyboard.

## Repo Map

- `boards/arm/eyelash_sofle/`: board DTS, defconfig, layouts, metadata, and module wiring.
- `config/`: user keymap, runtime config, and the pinned west manifest used by CI and local workspaces.
- `build.yaml`: canonical build matrix and artifact naming, including Studio, reset, and debug builds.
- `keymap_drawer.config.yaml`: keymap-drawer settings.
- `keymap-drawer/`: generated keymap diagrams and YAML output when those assets are regenerated.
- `.github/workflows/`: CI entry points for firmware builds.
- `.agentic/`: compact repo context, commands, and handoff docs for future agents.
- `scripts/agentic/verify.sh`: cheap structural validation used by `make verify`.

## Working Rules

- Treat `build.yaml` as the release contract. If build targets change, keep artifact names and docs aligned.
- Keep `config/west.yml` minimal and pinned to `v0.3`. Do not add donor display repos as runtime dependencies.
- When changing hardware behavior, inspect both the board files under `boards/arm/eyelash_sofle/` and the runtime config under `config/`.
- USB logging is enabled only through dedicated debug artifacts. Normal firmware builds should stay free of debug logging snippets.
- When planning or writing code, follow `.agentic/context/code-organization-convention.md`: keep a pure distinction between durable product/core code and temporary/mock code, assets, fixtures, or scaffolding.
- When adding or changing display-engine code under `display/`, follow `.agentic/context/display-engine-logging-convention.md` and update that convention if a new display subsystem needs different logging behavior.
- Treat the nice!view displays as portrait rectangles: top and bottom edges are short, left and right edges are long. The top status bar must be laid out across the narrow top edge.
- Treat `keymap-drawer/*.svg` and `keymap-drawer/*.yaml` as generated output if present. Update the source config first, then regenerate.
- Avoid editing workspace dependencies such as `.zmk/`, `zmk/`, `modules/`, or `.west/`. This repo should stay focused on the local board module and user config.

## Default Validation

- Run `make verify` after each change.
- If a disposable west workspace is available, follow `.agentic/commands.md` for local `west update` and build commands.
- For runtime debugging, flash one of the dedicated `*_debug` artifacts and inspect its USB serial logs directly.

## Change Checklist

Before handing off a change:

1. Confirm the touched files match the intended layer: board, config, workflow, docs, or generated assets.
2. Run `make verify`.
3. If `build.yaml` changed, update the relevant notes in `.agentic/commands.md` and `.agentic/context/repo-map.md`.
