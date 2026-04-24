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
  "build.yaml"
  "config/west.yml"
  "config/eyelash_sofle.conf"
  "config/eyelash_sofle.keymap"
  "config/eyelash_sofle.json"
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

require_file() {
  local path="$1"
  [[ -f "$ROOT_DIR/$path" ]] || fail "missing file: $path"
}

require_match() {
  local pattern="$1"
  local path="$2"
  rg -q --multiline "$pattern" "$ROOT_DIR/$path" || fail "expected pattern '$pattern' in $path"
}

require_no_match() {
  local pattern="$1"
  local path="$2"
  if rg -q --multiline "$pattern" "$ROOT_DIR/$path"; then
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
require_match 'artifact-name:\s+eyelash_sofle_left_studio_debug' 'build.yaml'
require_match 'artifact-name:\s+eyelash_sofle_right_debug' 'build.yaml'
require_match 'snippet:\s+studio-rpc-usb-uart' 'build.yaml'
require_match 'snippet:\s+zmk-usb-logging' 'build.yaml'
require_match 'CONFIG_ZMK_USB_LOGGING=y' 'build.yaml'
require_match 'CONFIG_ZMK_USB=y' 'build.yaml'

require_match 'name:\s+zmk' 'config/west.yml'
require_match 'revision:\s+v0\.3' 'config/west.yml'
require_match 'self:[[:space:]]+path:\s+config' 'config/west.yml'
require_no_match 'mario-peripheral-animation|nice-view-gem|zmk-nice-oled' 'config/west.yml'

require_match 'siblings:' 'boards/arm/eyelash_sofle/eyelash_sofle.zmk.yml'
require_match 'eyelash_sofle_left' 'boards/arm/eyelash_sofle/eyelash_sofle.zmk.yml'
require_match 'eyelash_sofle_right' 'boards/arm/eyelash_sofle/eyelash_sofle.zmk.yml'
require_match 'CONFIG_ZMK_DISPLAY=y' 'boards/arm/eyelash_sofle/eyelash_sofle_left_defconfig'
require_match 'CONFIG_ZMK_DISPLAY=y' 'boards/arm/eyelash_sofle/eyelash_sofle_right_defconfig'
require_match 'USB logging is enabled only through dedicated debug artifacts' 'AGENTS.md'

printf 'verify: ok\n'
