# Display Engine Increment 0 Audit

This note captures the repo wiring discovered before adding the dual nice!view scene engine.

## Build Contract

- `build.yaml` is the release contract.
- Normal firmware artifacts:
  - `eyelash_sofle_left`: `board: eyelash_sofle_left`, `shield: nice_view`
  - `eyelash_sofle_right`: `board: eyelash_sofle_right`, `shield: nice_view`
- Studio artifact:
  - `eyelash_sofle_left_studio`: `shield: nice_view`, `snippet: studio-rpc-usb-uart`
- Settings-reset artifacts:
  - `eyelash_sofle_left_settings_reset`: `shield: settings_reset`
  - `eyelash_sofle_right_settings_reset`: `shield: settings_reset`
- USB logging debug artifacts:
  - `eyelash_sofle_left_studio_debug`: `shield: nice_view`, Studio snippet, USB logging CMake args
  - `eyelash_sofle_right_debug`: `shield: nice_view`, `snippet: zmk-usb-logging`
- Do not add debug logging to normal firmware artifacts.
- Increment 1 should not require changing `build.yaml`.

## Manifest And Workspace

- `config/west.yml` is pinned to ZMK `v0.3`.
- The manifest has only the `zmkfirmware/zmk` project and `self.path: config`.
- Do not add donor display repos to `config/west.yml`.
- `.zmk/` is a local workspace and should not be edited as source of truth.
- In the current shell used during increment 0, `west` was not available on `PATH`, so firmware configure/build was not run.

## Existing nice!view Path

- The local repo is exposed to Zephyr as a board root via `zephyr/module.yml`:
  - `build.settings.board_root: .`
- Both halves enable display support in board defconfigs:
  - `boards/arm/eyelash_sofle/eyelash_sofle_left_defconfig`: `CONFIG_ZMK_DISPLAY=y`
  - `boards/arm/eyelash_sofle/eyelash_sofle_right_defconfig`: `CONFIG_ZMK_DISPLAY=y`
- The left half is the split central:
  - `boards/arm/eyelash_sofle/Kconfig.defconfig`: `CONFIG_ZMK_SPLIT_ROLE_CENTRAL default y` for `BOARD_EYELASH_SOFLE_LEFT`
- The shared board DTS provides the SPI bus expected by the upstream `nice_view` shield:
  - `boards/arm/eyelash_sofle/eyelash_sofle.dtsi`: `nice_view_spi: &spi0`
  - Pins: SCK `P0.20`, MOSI `P0.17`, MISO `P0.25`, CS `P0.6`
- The upstream ZMK `nice_view` shield overlay binds:
  - `compatible = "sharp,ls0xx"`
  - `width = <160>`
  - `height = <68>`
  - `zephyr,display = &nice_view`
- ZMK display init calls `zmk_display_status_screen()` and loads the returned LVGL screen.
- Upstream `nice_view` currently supplies a strong `zmk_display_status_screen()` through `custom_status_screen.c` when `CONFIG_NICE_VIEW_WIDGET_STATUS` is enabled.

## Local Insertion Points

Keep the dual-display engine local to this repo:

- `display/core/`
  Shared state types, mapping logic, scene selection, side policy, and plan building.
- `display/firmware/`
  ZMK-facing state adapters, event listeners, and optional debug helpers.
- `display/render/lvgl/`
  LVGL adapter that consumes the core plan and provides the firmware status screen entry point.
- `display/assets/`
  Placeholder frame sets and asset registry.
- `sim/`
  Ubuntu desktop simulator that shares `display/core/`.

## Firmware Wiring Notes

- The local LVGL renderer must avoid a duplicate strong `zmk_display_status_screen()` symbol with upstream `nice_view`.
- The clean path is to disable the upstream `NICE_VIEW_WIDGET_STATUS` for local scene-engine builds, then compile the local renderer as the status screen provider.
- Guard new firmware display sources so `settings_reset` builds do not pick up the engine.
- Preserve Studio and debug artifact behavior; they should use the same display engine as normal nice!view builds unless intentionally documented otherwise.

## Validation

- `make verify` passed during increment 0.
- `scripts/agentic/verify.sh` already checks the build matrix, the pinned `v0.3` manifest, absence of donor repos in `config/west.yml`, display enablement on both halves, and dedicated debug artifact policy.
