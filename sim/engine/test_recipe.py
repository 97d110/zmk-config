#!/usr/bin/env python3
#
# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import sys
from pathlib import Path


sys.dont_write_bytecode = True
sys.path.insert(0, str(Path(__file__).resolve().parent))

import test_timing  # noqa: E402  (reuses the host engine build + driver)


def recipe(snapshot: dict, side: str = "left") -> dict:
    return snapshot[side]["recipe"]  # type: ignore[index,return-value]


def commands(snapshot: dict, side: str = "left") -> list:
    return recipe(snapshot, side)["commands"]  # type: ignore[index,return-value]


def kinds(snapshot: dict, side: str = "left") -> list:
    return [c["kind"] for c in commands(snapshot, side)]


def count_kind(snapshot: dict, kind: str, side: str = "left") -> int:
    return sum(1 for c in commands(snapshot, side) if c["kind"] == kind)


def draw_sprites(snapshot: dict, side: str = "left") -> int:
    return count_kind(snapshot, "draw_sprite", side)


def phase(snapshot: dict, side: str = "left") -> str:
    return snapshot[side]["theme"]["phase"]  # type: ignore[index,return-value]


def galaxy_edge_frame(snapshot: dict, side: str = "left"):
    for c in commands(snapshot, side):
        if c["kind"] == "draw_clipped_sprite" and c["blend"] == "or_white":
            return c["frame"]
    return None


def check(cond: bool, message: str, snapshot: dict) -> None:
    if not cond:
        raise AssertionError(f"{message}: {snapshot}")


def run_tests() -> None:
    test_timing.build_engine()
    engine = test_timing.Engine()
    try:
        # Idle: base asteroid scene, no typing effects, actor present.
        idle = engine.last
        check(kinds(idle)[0] == "clear_region", "idle recipe must start with clear_region", idle)
        check(count_kind(idle, "draw_points") == 2, "idle draws far + mid star fields", idle)
        check(count_kind(idle, "draw_clipped_sprite") == 2, "idle draws two clipped galaxy layers",
              idle)
        check(count_kind(idle, "apply_clearance_mask") == 1, "idle clears under the actor", idle)
        check(count_kind(idle, "draw_sprite_masked") == 1, "idle draws the asteroid actor", idle)
        check(draw_sprites(idle) == 0, "idle has no typing effects", idle)

        # Peripheral (right) is a placeholder environment: no actor.
        check(kinds(idle, "right")[0] == "clear_region", "right starts with clear_region", idle)
        check(count_kind(idle, "draw_sprite_masked", "right") == 0, "right omits the actor", idle)
        check(count_kind(idle, "apply_clearance_mask", "right") == 0, "right omits clearance", idle)

        # Typing effects scale with phase.
        light = engine.command("key")
        check(phase(light) == "typing-light", "expected typing-light after a keypress", light)
        check(draw_sprites(light) == 2, "typing-light => 2 speed streaks", light)

        # Animated frames advance with the frame clock (two ticks apart differ).
        light2 = engine.command("key")
        check(galaxy_edge_frame(light) != galaxy_edge_frame(light2),
              "animated galaxy-edge frame should advance with ticks", light2)

        medium = test_timing.sustain_typing(engine, 5000)
        check(phase(medium) == "typing-medium", "expected typing-medium", medium)
        check(draw_sprites(medium) == 5, "typing-medium => 4 streaks + 1 twinkle", medium)

        high = test_timing.sustain_typing(engine, 7000)
        check(phase(high) == "typing-high", "expected typing-high", high)
        check(draw_sprites(high) == 8, "typing-high => 6 streaks + 2 twinkles", high)

        peak = test_timing.sustain_typing(engine, 6000)
        check(phase(peak) == "typing-peak", "expected typing-peak", peak)
        check(draw_sprites(peak) == 8, "typing-peak => 6 streaks + 2 twinkles", peak)

        # Decay reduces effects as the phase steps back down.
        decayed = engine.command("tick 1100")
        check(phase(decayed) == "typing-medium", "expected decay to typing-medium", decayed)
        check(draw_sprites(decayed) == 5, "decay to medium reduces effects", decayed)
        engine.command("tick 5000")  # -> typing-light
        idle_again = engine.command("tick 2000")  # -> idle
        check(phase(idle_again) == "idle", "expected decay back to idle", idle_again)
        check(draw_sprites(idle_again) == 0, "idle again => no typing effects", idle_again)

        # Visual display-sleep (phase sleep, scene still normal) => frozen black.
        slept = engine.command("tick 30000")
        check(phase(slept) == "sleep", "expected visual display-sleep", slept)
        check(recipe(slept)["commandCount"] == 1 and kinds(slept) == ["clear_region"],
              "visual sleep => clear-only recipe", slept)

        # ZMK global sleep (scene sleep) => frozen black too.
        gsleep = engine.command("sleep left 1")
        check(recipe(gsleep)["commandCount"] == 1 and kinds(gsleep) == ["clear_region"],
              "global sleep => clear-only recipe", gsleep)
    finally:
        engine.close()


def main() -> int:
    run_tests()
    print("sim recipe: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
