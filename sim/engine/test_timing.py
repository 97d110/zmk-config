#!/usr/bin/env python3
#
# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import importlib.util
import json
import shutil
import subprocess
import sys
from pathlib import Path


sys.dont_write_bytecode = True

ROOT_DIR = Path(__file__).resolve().parents[2]
TIMING_PROFILE = ROOT_DIR / "themes" / "space" / "v1" / "timing_profile.json"
TIMING_GENERATOR = ROOT_DIR / "scripts" / "agentic" / "generate_animation_timing.py"
GENERATED_INCLUDE_DIR = ROOT_DIR / "sim" / "build" / "generated-test"
GENERATED_HEADER = (
    GENERATED_INCLUDE_DIR / "display" / "render" / "animation" / "dual_display_animation_timing.h"
)
ENGINE_BIN = ROOT_DIR / "sim" / "build" / "dual_display_engine_test"
ENGINE_SOURCES = [
    ROOT_DIR / "sim" / "engine" / "dual_display_engine.c",
    ROOT_DIR / "display" / "core" / "dual_display_state.c",
    ROOT_DIR / "display" / "core" / "dual_display_plan.c",
    ROOT_DIR / "display" / "render" / "animation" / "dual_display_animation.c",
]


def load_timing_tools():
    spec = importlib.util.spec_from_file_location("generate_animation_timing", TIMING_GENERATOR)
    if spec is None or spec.loader is None:
        raise RuntimeError("cannot load theme timing generator")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def build_engine() -> None:
    cc = shutil.which("cc")
    if cc is None:
        raise RuntimeError("cc compiler is required for simulator timing tests")

    timing_tools = load_timing_tools()
    profile = timing_tools.load_profile(TIMING_PROFILE)
    timing_tools.write_header(profile, TIMING_PROFILE, GENERATED_HEADER)
    ENGINE_BIN.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(
        [
            cc,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-I",
            str(ROOT_DIR),
            "-I",
            str(GENERATED_INCLUDE_DIR),
            *[str(path) for path in ENGINE_SOURCES],
            "-o",
            str(ENGINE_BIN),
        ],
        cwd=ROOT_DIR,
        check=True,
    )


class Engine:
    def __init__(self) -> None:
        self.process = subprocess.Popen(
            [str(ENGINE_BIN)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        self.last = self._read()

    def _read(self) -> dict[str, object]:
        assert self.process.stdout is not None
        line = self.process.stdout.readline()
        if not line:
            stderr = self.process.stderr.read() if self.process.stderr is not None else ""
            raise RuntimeError(f"engine exited before snapshot; stderr={stderr}")
        return json.loads(line)

    def command(self, command: str) -> dict[str, object]:
        assert self.process.stdin is not None
        self.process.stdin.write(command + "\n")
        self.process.stdin.flush()
        self.last = self._read()
        return self.last

    def close(self) -> None:
        if self.process.stdin is not None:
            self.process.stdin.close()
        self.process.terminate()
        self.process.wait(timeout=2)


def left_theme(snapshot: dict[str, object]) -> dict[str, object]:
    return snapshot["left"]["theme"]  # type: ignore[index,return-value]


def assert_phase(snapshot: dict[str, object], phase: str) -> None:
    actual = left_theme(snapshot)["phase"]
    if actual != phase:
        raise AssertionError(f"expected phase {phase}, got {actual}: {snapshot}")


def assert_wants(snapshot: dict[str, object], expected: bool) -> None:
    actual = left_theme(snapshot)["wantsNextFrame"]
    if actual is not expected:
        raise AssertionError(f"expected wantsNextFrame {expected}, got {actual}: {snapshot}")


def sustain_typing(engine: Engine, total_ms: int) -> dict[str, object]:
    snapshot = engine.last
    elapsed = 0
    while elapsed < total_ms:
        snapshot = engine.command("key")
        step = min(1000, total_ms - elapsed)
        snapshot = engine.command(f"tick {step}")
        elapsed += step
    return snapshot


def run_tests() -> None:
    build_engine()
    engine = Engine()
    try:
        assert_phase(engine.last, "idle")
        assert_wants(engine.last, True)

        assert_phase(engine.command("key"), "typing-light")
        assert_phase(sustain_typing(engine, 5000), "typing-medium")
        assert_phase(sustain_typing(engine, 7000), "typing-high")
        assert_phase(sustain_typing(engine, 6000), "typing-peak")

        assert_phase(engine.command("tick 1100"), "typing-medium")
        assert_phase(engine.command("tick 5000"), "typing-light")
        assert_phase(engine.command("tick 2000"), "idle")
        assert_wants(engine.last, True)

        assert_phase(engine.command("tick 30000"), "sleep")
        assert_wants(engine.last, False)

        assert_phase(engine.command("key"), "typing-light")
        assert_wants(engine.last, True)

        assert_phase(engine.command("sleep left 1"), "sleep")
        assert_wants(engine.last, False)
    finally:
        engine.close()


def main() -> int:
    run_tests()
    print("sim timing: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
