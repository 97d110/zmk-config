#!/usr/bin/env python3
#
# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


FIELDS: tuple[str, ...] = (
    "frame_ms",
    "animation_loop_ms",
    "typing_light_ms",
    "typing_medium_ms",
    "typing_high_ms",
    "typing_peak_ms",
    "quiet_before_decay_ms",
    "decay_to_medium_ms",
    "decay_to_light_ms",
    "decay_to_idle_ms",
    "display_sleep_ms",
)

MACROS: dict[str, str] = {
    "frame_ms": "ZMK_DUAL_DISPLAY_THEME_FRAME_MS",
    "animation_loop_ms": "ZMK_DUAL_DISPLAY_THEME_ANIMATION_LOOP_MS",
    "typing_light_ms": "ZMK_DUAL_DISPLAY_THEME_TYPING_LIGHT_MS",
    "typing_medium_ms": "ZMK_DUAL_DISPLAY_THEME_TYPING_MEDIUM_MS",
    "typing_high_ms": "ZMK_DUAL_DISPLAY_THEME_TYPING_HIGH_MS",
    "typing_peak_ms": "ZMK_DUAL_DISPLAY_THEME_TYPING_PEAK_MS",
    "quiet_before_decay_ms": "ZMK_DUAL_DISPLAY_THEME_QUIET_BEFORE_DECAY_MS",
    "decay_to_medium_ms": "ZMK_DUAL_DISPLAY_THEME_DECAY_TO_MEDIUM_MS",
    "decay_to_light_ms": "ZMK_DUAL_DISPLAY_THEME_DECAY_TO_LIGHT_MS",
    "decay_to_idle_ms": "ZMK_DUAL_DISPLAY_THEME_DECAY_TO_IDLE_MS",
    "display_sleep_ms": "ZMK_DUAL_DISPLAY_THEME_DISPLAY_SLEEP_MS",
}


class ProfileError(ValueError):
    pass


def load_profile(path: Path) -> dict[str, int]:
    try:
        raw = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        raise ProfileError(f"{path}: invalid JSON: {exc}") from exc
    return validate_profile(raw)


def validate_profile(raw: Any) -> dict[str, int]:
    if not isinstance(raw, dict):
        raise ProfileError("timing profile must be a JSON object")

    extra = sorted(set(raw) - set(FIELDS))
    if extra:
        raise ProfileError(f"unknown timing profile field(s): {', '.join(extra)}")

    missing = [field for field in FIELDS if field not in raw]
    if missing:
        raise ProfileError(f"missing timing profile field(s): {', '.join(missing)}")

    profile: dict[str, int] = {}
    for field in FIELDS:
        value = raw[field]
        if not isinstance(value, int) or isinstance(value, bool):
            raise ProfileError(f"{field} must be an integer millisecond value")
        if value < 0:
            raise ProfileError(f"{field} must not be negative")
        if field in {"frame_ms", "animation_loop_ms", "display_sleep_ms"} and value == 0:
            raise ProfileError(f"{field} must be greater than zero")
        profile[field] = value

    if profile["typing_light_ms"] != 0:
        raise ProfileError("typing_light_ms must be 0; light starts on first observed typing plan")
    if not (
        profile["typing_light_ms"]
        <= profile["typing_medium_ms"]
        <= profile["typing_high_ms"]
        <= profile["typing_peak_ms"]
    ):
        raise ProfileError("typing thresholds must be monotonic")
    if not (
        profile["decay_to_medium_ms"]
        <= profile["decay_to_light_ms"]
        <= profile["decay_to_idle_ms"]
    ):
        raise ProfileError("decay thresholds must be monotonic")
    if profile["animation_loop_ms"] < profile["frame_ms"]:
        raise ProfileError("animation_loop_ms must be at least frame_ms")
    if profile["display_sleep_ms"] <= profile["decay_to_idle_ms"]:
        raise ProfileError("display_sleep_ms must be greater than decay_to_idle_ms")

    return profile


def format_profile_assignments(profile: dict[str, int]) -> str:
    return " ".join(f"{field}={profile[field]}" for field in FIELDS)


def header_text(profile: dict[str, int], source: Path) -> str:
    lines = [
        "/*",
        " * Copyright (c) 2026 The ZMK Contributors",
        " * SPDX-License-Identifier: MIT",
        " *",
        f" * Generated from {source.as_posix()}. Do not edit by hand.",
        " */",
        "",
        "#pragma once",
        "",
    ]
    for field in FIELDS:
        lines.append(f"#define {MACROS[field]} {profile[field]}U")
    lines.append("")
    return "\n".join(lines)


def write_header(profile: dict[str, int], source: Path, output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(header_text(profile, source), encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate display theme timing constants")
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--print-assignments", action="store_true")
    args = parser.parse_args()

    try:
        profile = load_profile(args.input)
        if args.output is not None:
            write_header(profile, args.input, args.output)
        if args.print_assignments:
            print(format_profile_assignments(profile))
    except ProfileError as exc:
        parser.exit(1, f"theme timing profile error: {exc}\n")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
