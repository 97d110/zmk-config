# Increment 2: State-Aware Status Planning

## Summary

Add the shared display state model and make status slots value-aware, while preserving the current portrait logical display contract, LVGL viewport mapper, and stable `screen_renderer.h` renderer boundary. Increment 2 will also update the mock renderer enough to visibly represent unknown values, battery buckets, transport, split-link, and layer placeholders.

## Key Changes

- Add durable core state/mapping under `display/core/`, with enums for role, battery bucket, activity bucket, transport, split link, and layer mode.
- Expand `status_slot_plan` from `kind + active` to `kind + value/state`, so renderers can distinguish known, disconnected, charging, and unknown values.
- Unknown status values render as the relevant base icon with a `/` overlay, not as a blank or disabled slot.
- Battery buckets are six states:
  - `0-10`, `11-50`, `51-100`
  - the same three ranges with charging overlay
- Activity buckets are:
  - `idle`, `sleep`, `typing_2s`, `typing_5s`, `typing_10s`, `typing_15s`
  - typing buckets are based on typing streak duration; `typing_15s` is the capped max bucket.
- Transport states are `usb`, `bt`, and `disconnected`; unknown transport uses the transport icon with `/`.
- Split link is represented as a wifi-like icon when connected; disconnected/unknown use the same base icon with `/`.
- Layer status shows the first letter of the keymap display name: `T`, `S`, `M`, `C`; unknown layer uses a layer icon with `/`.

## Implementation Shape

- Add state-aware planning APIs:
  - `zmk_dual_display_build_screen_plan_from_state(...)`
  - keep current side-only wrappers by constructing default side-derived state internally.
- Keep logical coordinates portrait: `68x160`, short top status bar, dominant lower animation region.
- Keep `display/render/lvgl/viewport.c` as the only portrait-to-framebuffer mapper.
- Keep firmware status screen pointed at `screen_renderer.h`; it must not include mock headers.
- Update `display/mock/lvgl/placeholder_renderer.c` to draw simple placeholder variants for the new slot values, including slash overlays.
- Add debug logs for mapping decisions, state transitions, and recoverable unknown/fallback cases; avoid per-frame logs.

## Validation

- Update `CMakeLists.txt` for the new core state source.
- Update `scripts/agentic/verify.sh` to require the new state files, state-aware planning API, mock renderer contract, and slash/fallback logging patterns.
- Add `.agentic/context/display-engine-increment-2.md` documenting state buckets, default values, renderer boundary, and validation.
- Run `make verify`.
- No planned changes to `build.yaml`, `config/west.yml`, Studio artifacts, settings-reset artifacts, or debug snippets.

## Assumptions

- Real ZMK event listeners remain increment 5 work.
- Default firmware state for increment 2 remains side-derived:
  - left = central, right = peripheral
  - layer = `Type`
  - activity = `idle`
  - battery, transport, and split-link = `unknown`
- Activity bucket affects status/state planning now; full animation switching by activity remains increment 3.
