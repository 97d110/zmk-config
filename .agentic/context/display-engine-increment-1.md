# Display Engine Increment 1 Handoff

Increment 1 adds the first local dual nice!view renderer slice.

## Implemented

- Added local Zephyr module wiring through root `Kconfig`, root
  `CMakeLists.txt`, and `zephyr/module.yml`.
- Added `CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE`, guarded by
  `SHIELD_NICE_VIEW && ZMK_DISPLAY`.
- Disabled upstream `NICE_VIEW_WIDGET_STATUS` in `config/eyelash_sofle.conf`
  so the local renderer provides the single strong `zmk_display_status_screen()`
  symbol.
- Added minimal LVGL-free core planning under `display/core/`.
- Added a local LVGL status-screen provider under `display/render/lvgl/`.
- Added placeholder asset identifiers under `display/assets/`.
- Added the display-engine logging convention and AGENTS rule.

## Logging

- Firmware logging module: `zmk_dual_display`.
- Runtime log level is controlled by
  `CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE_LOG_LEVEL`.
- Normal artifacts do not gain USB logging snippets or elevated log settings.
- Use the existing dedicated debug artifacts to view display-engine logs.

## Validation

- Run `make verify` after this increment.
- `west` was not available on the shell `PATH` during implementation, so local
  firmware builds could not be run from this environment.
