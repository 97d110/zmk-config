#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

required_files=(
  "AGENTS.md"
  "Makefile"
  ".agentic/README.md"
  ".agentic/context/repo-map.md"
  ".agentic/commands.md"
  ".agentic/tasks/TEMPLATE.md"
  ".agentic/checklists/change.md"
  ".agentic/context/code-organization-convention.md"
  ".agentic/context/display-engine-logging-convention.md"
  ".agentic/context/display-engine-increment-1.md"
  ".agentic/context/display-engine-increment-2.md"
  ".agentic/context/display-engine-increment-3.md"
  ".agentic/context/display-engine-increment-4.md"
  ".agentic/context/display-engine-increment-5.md"
  ".agentic/context/display-engine-increment-5A.md"
  ".agentic/context/display-engine-increment-5B.md"
  ".agentic/context/display-engine-increment-6.md"
  ".agentic/context/display-engine-increment-7.md"
  ".agentic/troubleshooting/split-pairing.md"
  "build.yaml"
  "CMakeLists.txt"
  "Kconfig"
  "docs/display-firmware-animation-flow.md"
  "config/west.yml"
  "config/eyelash_sofle.conf"
  "config/eyelash_sofle.keymap"
  "config/eyelash_sofle.json"
  "display/README.md"
  "display/assets/README.md"
  "display/core/dual_display_plan.c"
  "display/core/dual_display_plan.h"
  "display/core/dual_display_state.c"
  "display/core/dual_display_state.h"
  "display/core/README.md"
  "display/firmware/README.md"
  "display/firmware/dual_display_state_adapter.c"
  "display/firmware/dual_display_state_adapter.h"
  "display/log.h"
  "display/mock/README.md"
  "display/mock/lvgl/placeholder_renderer.c"
  "display/mock/lvgl/placeholder_renderer.h"
  "display/render/lvgl/README.md"
  "display/render/lvgl/dual_display_status_screen.h"
  "display/render/lvgl/screen_renderer.h"
  "display/render/lvgl/dual_display_status_screen.c"
  "display/render/lvgl/viewport.c"
  "display/render/lvgl/viewport.h"
  "display/render/theme/README.md"
  "display/render/theme/dual_display_theme.c"
  "display/render/theme/dual_display_theme.h"
  "display/render/theme/timing_profile.json"
  "scripts/agentic/generate_theme_timing.py"
  "sim/README.md"
  "sim/engine/dual_display_engine.c"
  "sim/engine/test_timing.py"
  "sim/web/app.py"
  "sim/web/Dockerfile"
  "sim/web/Makefile"
  "boards/arm/eyelash_sofle/Kconfig.board"
  "boards/arm/eyelash_sofle/Kconfig.defconfig"
  "boards/arm/eyelash_sofle/board.cmake"
  "boards/arm/eyelash_sofle/eyelash_sofle.dtsi"
  "boards/arm/eyelash_sofle/eyelash_sofle_left.dts"
  "boards/arm/eyelash_sofle/eyelash_sofle_right.dts"
  "boards/arm/eyelash_sofle/eyelash_sofle_left_defconfig"
  "boards/arm/eyelash_sofle/eyelash_sofle_right_defconfig"
  "boards/arm/eyelash_sofle/eyelash_sofle.zmk.yml"
  "keymap_drawer.config.yaml"
)

fail() {
  printf 'verify: %s\n' "$1" >&2
  exit 1
}

search_match() {
  local pattern="$1"
  local path="$2"

  if command -v rg >/dev/null 2>&1; then
    rg -q --multiline "$pattern" "$ROOT_DIR/$path"
    return $?
  fi

  if printf 'test' | grep -Pzq 't' >/dev/null 2>&1; then
    grep -Pzq "$pattern" "$ROOT_DIR/$path"
    return $?
  fi

  fail "requires rg or GNU grep with PCRE support"
}

require_file() {
  local path="$1"
  [[ -f "$ROOT_DIR/$path" ]] || fail "missing file: $path"
}

require_match() {
  local pattern="$1"
  local path="$2"
  search_match "$pattern" "$path" || fail "expected pattern '$pattern' in $path"
}

require_no_match() {
  local pattern="$1"
  local path="$2"
  if search_match "$pattern" "$path"; then
    fail "unexpected pattern '$pattern' in $path"
  fi
}

for path in "${required_files[@]}"; do
  require_file "$path"
done

require_match 'board:\s+eyelash_sofle_left' 'build.yaml'
require_match 'board:\s+eyelash_sofle_right' 'build.yaml'
require_match 'shield:\s+nice_view' 'build.yaml'
require_match 'shield:\s+settings_reset' 'build.yaml'
require_match 'artifact-name:\s+eyelash_sofle_left' 'build.yaml'
require_match 'artifact-name:\s+eyelash_sofle_right' 'build.yaml'
require_match 'artifact-name:\s+eyelash_sofle_left_studio' 'build.yaml'
require_match 'artifact-name:\s+eyelash_sofle_left_settings_reset' 'build.yaml'
require_match 'artifact-name:\s+eyelash_sofle_right_settings_reset' 'build.yaml'
require_match 'artifact-name:\s+eyelash_sofle_left_debug' 'build.yaml'
require_match 'artifact-name:\s+eyelash_sofle_left_studio_debug' 'build.yaml'
require_match 'artifact-name:\s+eyelash_sofle_right_debug' 'build.yaml'
require_match 'artifact-name:\s+eyelash_sofle_left_display_engine_debug' 'build.yaml'
require_match 'artifact-name:\s+eyelash_sofle_left_studio_display_engine_debug' 'build.yaml'
require_match 'artifact-name:\s+eyelash_sofle_right_display_engine_debug' 'build.yaml'
require_match 'snippet:\s+studio-rpc-usb-uart' 'build.yaml'
require_match 'snippet:\s+zmk-usb-logging' 'build.yaml'
require_match 'CONFIG_ZMK_USB_LOGGING=y' 'build.yaml'
require_match 'CONFIG_ZMK_USB=y' 'build.yaml'

require_match 'cmake:\s+\.' 'zephyr/module.yml'
require_match 'kconfig:\s+Kconfig' 'zephyr/module.yml'
require_match 'board_root:\s+\.' 'zephyr/module.yml'
require_match 'CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE' 'CMakeLists.txt'
require_match 'SHIELD_NICE_VIEW' 'Kconfig'
require_match 'ZMK_DISPLAY' 'Kconfig'
require_match 'LV_USE_CANVAS' 'Kconfig'
require_match 'module-str\s+=\s+zmk_dual_display' 'Kconfig'
require_no_match 'CONFIG_NICE_VIEW_WIDGET_STATUS=n' 'config/eyelash_sofle.conf'
require_match 'LOG_MODULE_REGISTER\(zmk_dual_display' 'display/render/lvgl/dual_display_status_screen.c'
require_match 'ZMK_DUAL_DISPLAY_LOG_DBG' 'display/core/dual_display_plan.c'
require_match 'display/core/dual_display_state.c' 'CMakeLists.txt'
require_match 'display/firmware/dual_display_state_adapter.c' 'CMakeLists.txt'
require_match 'display/render/theme/dual_display_theme.c' 'CMakeLists.txt'
require_match 'zmk_dual_display_build_screen_plan_from_state' 'display/core/dual_display_plan.h'
require_match 'zmk_dual_display_build_screen_plan_from_state' 'display/render/lvgl/dual_display_status_screen.c'
require_match 'zmk_dual_display_firmware_init_state' 'display/render/lvgl/dual_display_status_screen.c'
require_match 'zmk_dual_display_status_screen_update_from_state' 'display/render/lvgl/dual_display_status_screen.c'
require_match 'zmk_dual_display_status_screen_update_from_state' 'display/firmware/dual_display_state_adapter.c'
require_match 'ZMK_LISTENER\(dual_display_firmware_state' 'display/firmware/dual_display_state_adapter.c'
require_match 'ZMK_SUBSCRIPTION\(dual_display_firmware_state, zmk_activity_state_changed\)' 'display/firmware/dual_display_state_adapter.c'
require_match 'ZMK_SUBSCRIPTION\(dual_display_firmware_state, zmk_battery_state_changed\)' 'display/firmware/dual_display_state_adapter.c'
require_match 'ZMK_SUBSCRIPTION\(dual_display_firmware_state, zmk_endpoint_changed\)' 'display/firmware/dual_display_state_adapter.c'
require_match 'ZMK_SUBSCRIPTION\(dual_display_firmware_state, zmk_keycode_state_changed\)' 'display/firmware/dual_display_state_adapter.c'
require_match 'ZMK_SUBSCRIPTION\(dual_display_firmware_state, zmk_layer_state_changed\)' 'display/firmware/dual_display_state_adapter.c'
require_match 'ZMK_SUBSCRIPTION\(dual_display_firmware_state, zmk_split_peripheral_status_changed\)' 'display/firmware/dual_display_state_adapter.c'
require_match 'k_work_submit_to_queue\(zmk_display_work_q\(\), &firmware_render_work\)' 'display/firmware/dual_display_state_adapter.c'
require_match 'display state event produced no visual state change' 'display/firmware/dual_display_state_adapter.c'
require_match 'K_WORK_DELAYABLE_DEFINE\(typing_activity_work' 'display/firmware/dual_display_state_adapter.c'
require_match 'record_typing_keypress_locked' 'display/firmware/dual_display_state_adapter.c'
require_match 'complete display state' 'display/firmware/dual_display_state_adapter.c'
require_match 'typing-return-idle' 'display/firmware/dual_display_state_adapter.c'
require_match 'CONFIG_ZMK_DUAL_DISPLAY_TYPING_CHECK_PERIOD_MS' 'display/firmware/dual_display_state_adapter.c'
require_match 'CONFIG_ZMK_DUAL_DISPLAY_THEME_REFRESH_PERIOD_MS' 'display/firmware/dual_display_state_adapter.c'
require_match 'K_WORK_DELAYABLE_DEFINE\(theme_refresh_work' 'display/firmware/dual_display_state_adapter.c'
require_match 'zmk_dual_display_status_screen_next_frame_delay' 'display/firmware/dual_display_state_adapter.c'
require_match 'ZMK_DUAL_DISPLAY_BATTERY_0_10_CHARGING' 'display/core/dual_display_state.h'
require_match 'ZMK_DUAL_DISPLAY_ACTIVITY_TYPING' 'display/core/dual_display_state.h'
require_match 'enum zmk_dual_display_activity_state activity' 'display/core/dual_display_plan.h'
require_match 'zmk_dual_display_log_state_transition' 'display/core/dual_display_state.c'
require_match 'mapped invalid battery percent' 'display/core/dual_display_state.c'
require_match 'ZMK_DUAL_DISPLAY_SCENE_ENGINE_MOCK_RENDERER' 'Kconfig'
require_match 'ZMK_DUAL_DISPLAY_THEME_REFRESH_PERIOD_MS' 'Kconfig'
require_match 'display/mock/lvgl/placeholder_renderer.c' 'CMakeLists.txt'
require_match 'display/render/lvgl/viewport.c' 'CMakeLists.txt'
require_match 'zmk_dual_display_lvgl_render_screen_plan' 'display/render/lvgl/dual_display_status_screen.c'
require_match 'zmk_dual_display_lvgl_render_screen_plan' 'display/mock/lvgl/placeholder_renderer.c'
require_match 'lv_canvas_draw_rect' 'display/mock/lvgl/placeholder_renderer.c'
require_match 'slash overlay' 'display/mock/lvgl/placeholder_renderer.c'
require_match 'enum zmk_dual_display_scene_kind' 'display/core/dual_display_plan.h'
require_match 'ZMK_DUAL_DISPLAY_ENERGY_HIGH' 'display/core/dual_display_plan.h'
require_match 'ZMK_DUAL_DISPLAY_SCENE_SLEEP' 'display/core/dual_display_plan.c'
require_match 'ZMK_DUAL_DISPLAY_SCENE_LINK_ERROR' 'display/core/dual_display_plan.c'
require_match 'select_scene_kind' 'display/core/dual_display_plan.c'
require_match 'sleep override' 'display/core/dual_display_plan.c'
require_match 'link-error override' 'display/core/dual_display_plan.c'
require_match 'SCENE_SLEEP' 'display/mock/lvgl/placeholder_renderer.c'
require_match 'make sim' '.agentic/commands.md'
require_match 'make sim-web-docker' '.agentic/commands.md'
require_match 'Display Engine Increment 4 Handoff' '.agentic/context/display-engine-increment-4.md'
require_match 'Display Engine Increment 5 Handoff' '.agentic/context/display-engine-increment-5.md'
require_match 'Display Engine Increment 5\.A Handoff' '.agentic/context/display-engine-increment-5A.md'
require_match 'typing-return-idle' '.agentic/context/display-engine-increment-5A.md'
require_match 'Display Engine Increment 5\.B Handoff' '.agentic/context/display-engine-increment-5B.md'
require_match 'core logical display state' '.agentic/context/display-engine-increment-5B.md'
require_match 'theme animation state' '.agentic/context/display-engine-increment-5B.md'
require_match 'Display Engine Increment 6 Handoff' '.agentic/context/display-engine-increment-6.md'
require_match 'display/render/theme/' '.agentic/context/display-engine-increment-6.md'
require_match 'display/firmware/' 'display/README.md'
require_match 'browser canvas app' 'sim/README.md'
require_match 'local Python serial bridge' 'sim/README.md'
require_match 'host C runner' 'sim/README.md'
require_match 'zmk_dual_display_theme_context_observe_plan' 'sim/engine/dual_display_engine.c'
require_match 'zmk_dual_display_build_dual_plan_from_state' 'sim/engine/dual_display_engine.c'
require_match 'dual display canvas simulator' 'sim/web/app.py'
require_match 'SerialTailer' 'sim/web/app.py'
require_match 'EngineProcess' 'sim/web/app.py'
require_match 'KeyboardEventController' 'sim/web/app.py'
require_match 'layer_changed' 'sim/web/app.py'
require_match '_active_layers' 'sim/web/app.py'
require_match '_source_sides' 'sim/web/app.py'
require_match '_mark_usb_charging' 'sim/web/app.py'
require_match '_apply_usb_power_from_sources' 'sim/web/app.py'
require_match 'visible_sources' 'sim/web/app.py'
require_match '/api/serial' 'sim/web/app.py'
require_match 'snapshot' 'sim/web/app.py'
require_no_match 'requestPort|navigator\.serial|manualOverride|Manual State|Connect Serial' 'sim/web/app.py'
require_match '<canvas id="leftCanvas"' 'sim/web/app.py'
require_match '<canvas id="rightCanvas"' 'sim/web/app.py'
require_match 'lastKeyboardStateAt' 'sim/web/app.py'
require_match 'docker-run' 'sim/web/Makefile'
require_match 'build-base' 'sim/web/Dockerfile'
require_no_match 'dual_display_sim|ASCII|terminal preview' 'sim/README.md'
require_no_match 'sim-build|sim-clean' 'Makefile'
require_match 'struct zmk_dual_display_theme_context' 'display/render/theme/dual_display_theme.h'
require_match 'ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_PEAK' 'display/render/theme/dual_display_theme.h'
require_match 'struct zmk_dual_display_theme_timing_profile' 'display/render/theme/dual_display_theme.h'
require_match 'zmk_dual_display_theme_context_observe_plan_elapsed' 'display/render/theme/dual_display_theme.h'
require_match 'timing_profile\.json' 'CMakeLists.txt'
require_match 'generate_theme_timing\.py' 'CMakeLists.txt'
require_match 'dual_display_theme_timing\.h' 'CMakeLists.txt'
require_match 'wants_next_frame' 'display/render/lvgl/screen_renderer.h'
require_match 'ZMK_DUAL_DISPLAY_SIM_LOG' 'display/log.h'
require_match 'zmk_dual_display_lvgl_map_rect' 'display/render/lvgl/viewport.c'
require_match 'ZMK_DUAL_DISPLAY_HEIGHT - bounds->y - bounds->height' 'display/render/lvgl/viewport.c'
require_match '\.y = bounds->x' 'display/render/lvgl/viewport.c'
require_no_match 'display/mock' 'display/render/lvgl/dual_display_status_screen.c'
require_match 'Temporary placeholder drawings' 'display/assets/README.md'
require_match 'This subtree is temporary by design' 'display/mock/README.md'
require_match 'code-organization-convention\.md' 'AGENTS.md'
require_match 'durable product/core code and temporary/mock code' 'AGENTS.md'
require_match 'Code organization convention for increments 1-7' '.codex/zmk_dual_shield_animation_tech_spec_prompt_v3.md'
require_match 'display-engine-logging-convention\.md' 'AGENTS.md'
require_match 'Typing Lifecycle' 'docs/display-firmware-animation-flow.md'
require_match 'zmk_dual_display_firmware_init_state' 'docs/display-firmware-animation-flow.md'
require_match 'firmware_state_listener_cb' 'docs/display-firmware-animation-flow.md'
require_match 'typing_activity_work_cb' 'docs/display-firmware-animation-flow.md'
require_match '```mermaid' 'docs/display-firmware-animation-flow.md'
require_match 'display-firmware-animation-flow\.md' '.agentic/context/repo-map.md'
require_match 'display-engine-increment-7\.md' '.agentic/context/repo-map.md'
require_match 'Display Engine Increment 7 Handoff' '.agentic/context/display-engine-increment-7.md'
require_match 'Timing Profile' 'docs/display-firmware-animation-flow.md'
require_match 'Display Plan' 'docs/display-firmware-animation-flow.md'
require_match 'Theme State / Animation State' 'docs/display-firmware-animation-flow.md'
require_match 'make sim-test' '.agentic/commands.md'
require_match 'make sim-test' 'sim/README.md'

require_match 'name:\s+zmk' 'config/west.yml'
require_match 'revision:\s+v0\.3' 'config/west.yml'
require_match 'self:[[:space:]]+path:\s+config' 'config/west.yml'
require_no_match 'mario-peripheral-animation|nice-view-gem|zmk-nice-oled' 'config/west.yml'

require_match 'siblings:' 'boards/arm/eyelash_sofle/eyelash_sofle.zmk.yml'
require_match 'eyelash_sofle_left' 'boards/arm/eyelash_sofle/eyelash_sofle.zmk.yml'
require_match 'eyelash_sofle_right' 'boards/arm/eyelash_sofle/eyelash_sofle.zmk.yml'
require_match 'CONFIG_ZMK_DISPLAY=y' 'boards/arm/eyelash_sofle/eyelash_sofle_left_defconfig'
require_match 'CONFIG_ZMK_DISPLAY=y' 'boards/arm/eyelash_sofle/eyelash_sofle_right_defconfig'
require_match 'zephyr,console = &cdc_acm_uart' 'boards/arm/eyelash_sofle/eyelash_sofle.dtsi'
require_match 'USB logging is enabled only through dedicated debug artifacts' 'AGENTS.md'
require_match 'eyelash_sofle_right_settings_reset' '.agentic/troubleshooting/split-pairing.md'
require_match 'central-only reset is incomplete' '.agentic/troubleshooting/split-pairing.md'

tmp_header="$(mktemp -t zmk-dual-display-theme-timing-XXXXXX.h)"
python3 "$ROOT_DIR/scripts/agentic/generate_theme_timing.py" \
  --input "$ROOT_DIR/display/render/theme/timing_profile.json" \
  --output "$tmp_header"
if ! grep -q 'ZMK_DUAL_DISPLAY_THEME_TYPING_PEAK_MS 18000U' "$tmp_header"; then
  fail "generated timing header did not contain expected peak threshold"
fi
rm -f "$tmp_header"

python3 "$ROOT_DIR/sim/engine/test_timing.py" >/dev/null

printf 'verify: ok\n'
