# Display Engine Logging Convention

This convention applies whenever an agent adds or changes code under
`display/` for the local dual nice!view scene engine.

## Module Names

- Use one firmware logging module for the display engine:
  `zmk_dual_display`.
- Register it once in the firmware/LVGL entry point with:
  `LOG_MODULE_REGISTER(zmk_dual_display, CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE_LOG_LEVEL)`.
- Other display-engine C files should include `display/log.h` and use the
  `ZMK_DUAL_DISPLAY_LOG_*` wrappers.

## Level Rules

- Use `LOG_DBG`/`ZMK_DUAL_DISPLAY_LOG_DBG` for normal trace points:
  initialization, side selection, state mapping, plan construction, render
  refreshes, geometry choices, and placeholder or asset selection.
- Use `LOG_WRN`/`ZMK_DUAL_DISPLAY_LOG_WRN` for unexpected but recoverable
  fallbacks, invalid enum values, missing state, or null output buffers.
- Use `LOG_ERR`/`ZMK_DUAL_DISPLAY_LOG_ERR` only for failures that prevent the
  display engine from producing a usable screen.
- Do not use `printk` in display-engine code.

## Hot Path Rules

- Do not add unconditional logs inside timer, animation frame, or redraw loops.
- Per-frame logs must be gated by state changes or by future explicit debug
  controls.
- Normal builds must not enable extra USB logging snippets or elevated log
  levels. Runtime logs are surfaced through the existing dedicated debug
  artifacts in `build.yaml`.

## Future Increment Requirement

Every display-engine increment must either add useful diagnostics for the new
logic or explicitly state why no new trace point is needed. If a new subsystem
needs different behavior, update this convention in the same change.
