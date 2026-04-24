# Bootstrap `zmk-config` as the Eyelash Sofle Project

## Summary
- Turn `zmk-config` into the single source of truth for the Eyelash Sofle board module and user config, pinned to ZMK `v0.3`.
- Migrate the existing board definition, keymap, and hardware feature config from `zmk-sofle-choc-ali-expressed`, but do not carry over donor display modules as runtime dependencies.
- Make both halves first-class nice!view targets in the initial repo, while keeping the display implementation at the stock/base nice!view level for now.
- Preserve the utility artifacts from the old setup, add explicit debug artifacts for runtime logging, and finish by initializing agent documentation for efficient ongoing work.
- Define “working” as: local validation passes, GitHub Actions builds succeed, real hardware regression passes, and the repo contains concise agent-facing docs that describe structure, commands, and maintenance rules.

## Implementation Changes
- Keep `config/west.yml` minimal and pinned to `v0.3`, with only the upstream `zmk` project plus `self.path: config`. Do not add `mario-peripheral-animation`, `nice-view-gem`, `zmk-nice-oled`, or other donor repos.
- Keep `zephyr/module.yml` with `board_root: .` and use this repo as the out-of-tree board module host.
- Replace the stock placeholder `boards/shields/.gitkeep` setup with the real board module under `boards/arm/eyelash_sofle/`, migrated from the old repo.
- Migrate the current board metadata and board siblings intact: `eyelash_sofle`, `eyelash_sofle_left`, and `eyelash_sofle_right` remain the canonical identifiers.
- Migrate the current user config files into `config/`: `eyelash_sofle.conf`, `eyelash_sofle.keymap`, and `eyelash_sofle.json`, preserving the existing keymap and hardware behavior defaults.
- Update the board/config baseline so both left and right halves are nice!view-capable in this repo from the start. This includes enabling display support on the right-side board path, not just the left.
- Keep the initial display scope intentionally narrow: stock nice!view behavior only, no local dual-scene engine yet, no donor-runtime UI stack, and no animation-engine work in this migration.
- Rebuild `build.yaml` as the release contract for the new repo. It should include:
  - left firmware with nice!view
  - right firmware with nice!view
  - left Studio build with `studio-rpc-usb-uart`
  - left settings-reset build
  - left debug build with `zmk-usb-logging`
  - right debug build with `zmk-usb-logging`
- Add a debug-only logging policy to the repo:
  - USB logging is enabled only through dedicated debug artifacts, never in the normal daily firmware builds
  - runtime instrumentation should use Zephyr/ZMK logging macros in custom code paths later, especially display-state gathering and scene-planning code
  - no attempt is made to relay peripheral logs through the central; each half is debugged from its own USB logging build
- Carry over lightweight repo validation from the old repo so the new project has a concrete `make verify` contract. The verify target should check the presence and consistency of the pinned manifest, board module, keymap/config files, and required build artifacts in `build.yaml`.
- Add a final documentation bootstrap step for agent efficiency:
  - create `AGENTS.md` at repo root with repo map, working rules, validation expectations, and change checklist tailored to `zmk-config`
  - create a minimal `.agentic/` set with `README.md`, `commands.md`, `context/repo-map.md`, `checklists/change.md`, and `tasks/TEMPLATE.md`
  - document the canonical build matrix, local verification commands, flash/debug workflow, and which paths are authoritative for board, config, generated assets, and future display-engine work
  - keep these docs short and operational so future agents can gather context without rereading the whole repo

## Public Interfaces And Contracts
- Board/module identity stays `eyelash_sofle` with siblings `eyelash_sofle_left` and `eyelash_sofle_right`.
- Build contract changes from the old asymmetric setup to a symmetric split-display setup: both normal firmware artifacts are nice!view builds.
- Manifest contract is explicitly pinned: `zmk` on `v0.3`, no runtime donor modules.
- Display contract for this migration is “base nice!view on both halves”; future custom display-engine work is intentionally deferred and should not leak into this bootstrap.
- Utility artifact contract is preserved and extended:
  - Studio-enabled left firmware remains supported
  - settings-reset remains supported
  - left and right debug firmware artifacts are first-class supported outputs for runtime inspection
- Debug contract is USB logging over dedicated builds, not cross-half log relay.
- Agent-doc contract is explicit: `AGENTS.md` and `.agentic/` become the maintained source for repo context, commands, and handoff expectations.

## Test Plan
- Local validation:
  - `make verify` passes in the new repo
  - `west update` completes using only pinned upstream ZMK plus the local module
  - local builds succeed for every required matrix entry in `build.yaml`, including Studio, reset, and both USB-logging debug builds
- CI validation:
  - GitHub Actions build workflow succeeds for every artifact in the matrix
  - artifact names and matrix contents match the intended release contract
- Hardware regression:
  - left and right halves both boot and pair/connect correctly as a split
  - both nice!view displays initialize and show the base display behavior
  - existing keymap behavior matches the prior repo on all intended layers
  - encoder behavior works as before
  - pointing behavior works as before
  - RGB underglow and backlight behave as before
  - Studio-enabled left firmware connects and functions
  - settings-reset artifact boots and performs the expected reset workflow
  - left debug artifact exposes USB logging and produces runtime logs over serial
  - right debug artifact exposes USB logging and produces runtime logs over serial
- Documentation validation:
  - `make verify` checks for the required agent-doc files
  - the documented commands and repo map match the actual migrated layout and build matrix
- Migration completion criteria:
  - the new repo can be used as the primary build/flash source for the keyboard without depending on the old repo or donor display modules
  - the repo includes a reliable debug path for runtime inspection on either half
  - the repo includes concise agent documentation sufficient for future work without re-deriving the project structure

## Assumptions And Defaults
- This repo is the long-term project home for board, config, and later local display architecture.
- The initial migration is for functional parity and repo ownership, not for implementing the new dual-display scene engine.
- Both halves are treated as immediate nice!view targets even though the old repo only built the left half with nice!view.
- Donor repos remain references for later design work only; they are not part of the runtime dependency graph.
- ZMK `v0.3` is the default pinned line for this migration, and no `main`-branch/HWMv2 work is included in this plan.
- Runtime debugging is done by flashing a dedicated debug image to the half being inspected and reading logs from its USB serial device.
- Agent documentation is created after the repo structure and build contract are stable enough to document accurately, but before the migration is considered complete.
