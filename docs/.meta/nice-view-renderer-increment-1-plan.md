# Increment 1: Minimal Renderer With Logging Convention

## Summary
Add the first local dual nice!view renderer slice, plus a repo-level logging convention and agent rule so future display-engine increments consistently add useful diagnostics without changing normal build behavior.

## Key Changes
- Add local Zephyr module wiring for `CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE`, guarded by `SHIELD_NICE_VIEW && ZMK_DISPLAY`.
- Disable upstream nice!view status widget with `CONFIG_NICE_VIEW_WIDGET_STATUS=n`; local renderer provides `zmk_display_status_screen()`.
- Add minimal `display/core/` plan types and `display/render/lvgl/` renderer for:
  - fixed `160x68` layout,
  - top status bar,
  - lower placeholder animation region,
  - visibly distinct left/right placeholder composition.
- Add logging throughout new logic:
  - `LOG_MODULE_REGISTER(zmk_dual_display, CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE_LOG_LEVEL)` in firmware/LVGL code.
  - `LOG_DBG` for init, plan construction, side selection, render refresh, geometry decisions, and placeholder asset selection.
  - `LOG_WRN` for unexpected or fallback paths.
  - No `printk`, no unconditional hot-loop logging, and no debug snippet/log-level changes in normal artifacts.

## Documentation And Agent Rule
- Add `.agentic/context/display-engine-logging-convention.md` documenting:
  - module names,
  - log level rules,
  - what every new display-engine file should log,
  - what must stay quiet in render/timer hot paths,
  - how logs are surfaced through existing debug artifacts.
- Update `AGENTS.md` with a code-writing rule:
  - when adding or changing display-engine code, the agent must follow the logging convention and update the convention if new subsystems need different logging behavior.
- Update the tech spec file under `.codex/` so increments 2-7 explicitly inherit the logging convention and include logging in their acceptance criteria.

## Build And Workflow Impact
- Do not change `build.yaml`.
- Do not change `config/west.yml`.
- Normal builds remain free of USB logging snippets and elevated debug settings.
- Studio/debug nice!view artifacts compile the same engine; dedicated debug artifacts remain the intended way to view runtime logs.
- Settings-reset artifacts do not compile the engine.

## Test Plan
- Run `make verify`.
- Extend `scripts/agentic/verify.sh` to check:
  - local module CMake/Kconfig wiring exists,
  - `CONFIG_NICE_VIEW_WIDGET_STATUS=n` is set,
  - logging convention doc exists,
  - `AGENTS.md` mentions the display-engine logging rule,
  - donor repos remain absent from `config/west.yml`,
  - `build.yaml` artifact contract is unchanged.
- If `west` is available, build normal left/right nice_view, left Studio nice_view, and both settings-reset artifacts.
- If `west` is unavailable, record that limitation in the increment handoff note.

## Assumptions
- “Logs throughout” means debug-level instrumentation in the new code, not enabling verbose logging in normal firmware.
- Increment 1 still avoids real ZMK state listeners, simulator work, final art, and donor assets.
