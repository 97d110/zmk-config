#!/usr/bin/env python3
#
# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import glob
import json
import os
import re
import select
import shutil
import subprocess
import termios
import threading
import time
from collections import deque
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import parse_qs, urlparse


ROOT_DIR = Path(__file__).resolve().parents[2]
PORT = int(os.environ.get("PORT", "8080"))
SERIAL_GLOBS = ("/dev/serial/by-id/*", "/dev/ttyACM*")
ENGINE_BIN = ROOT_DIR / "sim" / "build" / "dual_display_engine"
ENGINE_SOURCES = [
    ROOT_DIR / "sim" / "engine" / "dual_display_engine.c",
    ROOT_DIR / "display" / "core" / "dual_display_state.c",
    ROOT_DIR / "display" / "core" / "dual_display_plan.c",
    ROOT_DIR / "display" / "render" / "theme" / "dual_display_theme.c",
]


class EngineProcess:
    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._process: subprocess.Popen[str] | None = None
        self._last_snapshot: dict[str, object] = {}
        self._last_tick_at = time.monotonic()

    def start(self) -> None:
        self._build()
        self._process = subprocess.Popen(
            [str(ENGINE_BIN)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
        )
        self._last_snapshot = self._read_snapshot()

    def snapshot(self) -> dict[str, object]:
        with self._lock:
            return dict(self._last_snapshot)

    def tick(self) -> dict[str, object]:
        now = time.monotonic()
        elapsed_ms = max(1, int((now - self._last_tick_at) * 1000))
        self._last_tick_at = now
        return self.command(f"tick {elapsed_ms}")

    def command(self, command: str) -> dict[str, object]:
        with self._lock:
            if self._process is None or self._process.stdin is None:
                return dict(self._last_snapshot)
            self._process.stdin.write(command + "\n")
            self._process.stdin.flush()
            self._last_snapshot = self._read_snapshot()
            return dict(self._last_snapshot)

    def _build(self) -> None:
        if shutil.which("cc") is None:
            raise RuntimeError("cc compiler is required for the host display engine")
        ENGINE_BIN.parent.mkdir(parents=True, exist_ok=True)
        newest_source = max(path.stat().st_mtime for path in ENGINE_SOURCES)
        if ENGINE_BIN.exists() and ENGINE_BIN.stat().st_mtime >= newest_source:
            return
        subprocess.run(
            [
                "cc",
                "-std=c11",
                "-Wall",
                "-Wextra",
                "-I",
                str(ROOT_DIR),
                *[str(path) for path in ENGINE_SOURCES],
                "-o",
                str(ENGINE_BIN),
            ],
            cwd=ROOT_DIR,
            check=True,
        )

    def _read_snapshot(self) -> dict[str, object]:
        if self._process is None or self._process.stdout is None:
            return {}
        line = self._process.stdout.readline()
        if not line:
            return {}
        return json.loads(line)


class KeyboardEventController:
    def __init__(self, engine: EngineProcess) -> None:
        self._engine = engine
        self._lock = threading.Lock()
        self._cursor = 0
        self._active_layers: set[int] = {0}
        self._known_sources: set[str] = set()
        self._source_sides: dict[str, str] = {}
        self._charging_sides: set[str] = set()

    def apply_entries(self, entries: list[dict[str, object]],
                      visible_sources: list[str] | None = None) -> dict[str, object]:
        with self._lock:
            if visible_sources is not None:
                self._known_sources.update(source for source in visible_sources if source)
                self._apply_usb_power_from_sources()
            for entry in entries:
                entry_id = int(entry["id"])
                if entry_id <= self._cursor:
                    continue
                self._cursor = entry_id
                source = str(entry.get("source", ""))
                if source:
                    self._known_sources.add(source)
                self._apply_line(str(entry["line"]), source)
                self._infer_unmapped_sources()
            self._apply_usb_power_from_sources()
            return self._engine.tick()

    def _apply_line(self, line: str, source: str) -> None:
        self._apply_source_side_hint(line, source)
        if "zmk_dual_display" in line:
            return
        if self._apply_layer_state_change(line):
            return
        if self._is_key_event(line) and self._is_key_press(line):
            self._engine.command("key")
            return
        layer = self._extract_field(line, "layer")
        if layer is not None and self._is_core_layer_line(line):
            self._engine.command(f"layer {layer}")

    def _apply_source_side_hint(self, line: str, source: str) -> None:
        if not source:
            return

        side = self._side_from_line(line)
        if side is None:
            return

        self._source_sides[source] = side
        self._mark_usb_charging(side)

    def _side_from_line(self, line: str) -> str | None:
        match = re.search(r"side=(left|right)", line)
        if match is not None:
            return match.group(1)
        match = re.search(r"firmware side selected from board config:\s+(left|right)", line)
        if match is not None:
            return match.group(1)
        return None

    def _infer_unmapped_sources(self) -> None:
        if len(self._known_sources) < 2 or "left" not in self._source_sides.values():
            return
        for source in self._known_sources:
            if source not in self._source_sides:
                self._source_sides[source] = "right"
                self._mark_usb_charging("right")

    def _apply_usb_power_from_sources(self) -> None:
        if len(self._known_sources) >= 2:
            self._mark_usb_charging("left")
            self._mark_usb_charging("right")

    def _mark_usb_charging(self, side: str) -> None:
        if side in self._charging_sides:
            return
        self._charging_sides.add(side)
        self._engine.command(f"battery {side} 100 1")

    def _apply_layer_state_change(self, line: str) -> bool:
        match = re.search(r"layer_changed:\s+layer\s+(\d+)\s+state\s+([01])", line)
        if match is None:
            return False

        layer = int(match.group(1))
        active = match.group(2) == "1"
        if active:
            self._active_layers.add(layer)
        else:
            self._active_layers.discard(layer)
        self._active_layers.add(0)
        self._engine.command(f"layer {max(self._active_layers)}")
        return True

    def _is_key_event(self, line: str) -> bool:
        lowered = line.lower()
        if "pressed: true" in lowered or "state on" in lowered:
            return True
        if "keycode" in lowered and "state=1" in lowered:
            return True
        return False

    def _is_key_press(self, line: str) -> bool:
        lowered = line.lower()
        return not any(token in lowered for token in ("state=0", "pressed=false", "release", "released"))

    def _is_core_layer_line(self, line: str) -> bool:
        lowered = line.lower()
        return "layer" in lowered and "zmk_dual_display" not in lowered

    def _extract_field(self, line: str, field: str) -> str | None:
        needle = field + "="
        index = line.find(needle)
        if index < 0:
            return None
        start = index + len(needle)
        end = start
        while end < len(line) and line[end].isdigit():
            end += 1
        return line[start:end] if end > start else None


class SerialTailer:
    def __init__(self) -> None:
        self._lock = threading.Lock()
        self._entries: deque[dict[str, object]] = deque(maxlen=1200)
        self._ports: dict[str, dict[str, object]] = {}
        self._errors: dict[str, str] = {}
        self._next_id = 1
        self._thread = threading.Thread(target=self._run, daemon=True)

    def start(self) -> None:
        self._thread.start()

    def snapshot(self, since: int) -> dict[str, object]:
        with self._lock:
            entries = [entry for entry in self._entries if int(entry["id"]) > since]
            cursor = self._next_id - 1
            ports = sorted(self._ports.keys())
            errors = dict(sorted(self._errors.items()))
        return {"cursor": cursor, "entries": entries, "ports": ports, "errors": errors}

    def _append_line(self, source: str, line: str) -> None:
        clean = line.strip()
        if not clean:
            return
        with self._lock:
            self._entries.append({"id": self._next_id, "source": source, "line": clean})
            self._next_id += 1

    def _candidate_paths(self) -> list[str]:
        paths: list[str] = []
        seen_realpaths: set[str] = set()
        for pattern in SERIAL_GLOBS:
            for path in sorted(glob.glob(pattern)):
                realpath = os.path.realpath(path)
                if realpath in seen_realpaths:
                    continue
                seen_realpaths.add(realpath)
                paths.append(path)
        return paths

    def _configure_tty(self, fd: int) -> None:
        attrs = termios.tcgetattr(fd)
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] |= termios.CLOCAL | termios.CREAD
        attrs[3] = 0
        attrs[4] = termios.B115200
        attrs[5] = termios.B115200
        termios.tcsetattr(fd, termios.TCSANOW, attrs)

    def _open_port(self, path: str) -> None:
        with self._lock:
            if path in self._ports:
                return
        try:
            fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK | os.O_NOCTTY)
            try:
                self._configure_tty(fd)
            except termios.error:
                pass
        except OSError as exc:
            with self._lock:
                self._errors[path] = str(exc)
            return
        with self._lock:
            self._ports[path] = {"fd": fd, "buffer": b""}
            self._errors.pop(path, None)

    def _close_port(self, path: str, reason: str) -> None:
        with self._lock:
            port = self._ports.pop(path, None)
            self._errors[path] = reason
        if port is not None:
            try:
                os.close(int(port["fd"]))
            except OSError:
                pass

    def _scan(self) -> None:
        current = set(self._candidate_paths())
        with self._lock:
            open_paths = set(self._ports.keys())
        for stale in open_paths - current:
            self._close_port(stale, "device disappeared")
        for path in current:
            self._open_port(path)

    def _read_ready_ports(self) -> None:
        with self._lock:
            fd_to_path = {int(port["fd"]): path for path, port in self._ports.items()}
        if not fd_to_path:
            time.sleep(0.15)
            return
        try:
            ready, _, _ = select.select(list(fd_to_path.keys()), [], [], 0.15)
        except OSError:
            return
        for fd in ready:
            path = fd_to_path[fd]
            try:
                data = os.read(fd, 4096)
            except BlockingIOError:
                continue
            except OSError as exc:
                self._close_port(path, str(exc))
                continue
            if not data:
                continue
            with self._lock:
                port = self._ports.get(path)
                if port is None:
                    continue
                buffer = bytes(port["buffer"]) + data
                lines = buffer.splitlines(keepends=True)
                if lines and not lines[-1].endswith((b"\n", b"\r")):
                    port["buffer"] = lines.pop()
                else:
                    port["buffer"] = b""
            for raw in lines:
                self._append_line(path, raw.decode("utf-8", errors="replace"))

    def _run(self) -> None:
        last_scan = 0.0
        while True:
            now = time.monotonic()
            if now - last_scan > 1.0:
                self._scan()
                last_scan = now
            self._read_ready_ports()


SERIAL_TAILER = SerialTailer()
ENGINE = EngineProcess()
KEYBOARD_CONTROLLER = KeyboardEventController(ENGINE)


PAGE = r"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Eyelash Sofle Display Simulator</title>
  <style>
    :root {
      color-scheme: dark;
      --bg: #0d1112;
      --panel: #151b1d;
      --panel-2: #1d2528;
      --line: #334044;
      --text: #e9f0ed;
      --muted: #9aa9a5;
      --accent: #59c79b;
      --warn: #efc261;
      --danger: #ff756e;
      --ink: #111;
      --paper: #eef5ef;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      background: var(--bg);
      color: var(--text);
      font: 14px/1.45 system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    }
    main {
      display: grid;
      grid-template-columns: minmax(0, 1fr);
      min-height: 100vh;
      padding: 16px;
    }
    aside, section { min-width: 0; }
    .panel {
      border: 1px solid var(--line);
      background: var(--panel);
    }
    .workspace {
      display: grid;
      grid-template-rows: auto minmax(420px, 1fr) minmax(180px, 30vh);
      gap: 14px;
      min-height: calc(100vh - 32px);
    }
    .statusbar {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 10px 12px;
      color: var(--muted);
      font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
      overflow: auto;
    }
    .canvas-wrap {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 18px;
      align-items: center;
      justify-items: center;
      padding: 18px;
      background: #090c0d;
      overflow: auto;
    }
    .display {
      display: grid;
      gap: 10px;
      justify-items: center;
    }
    .display-title {
      color: var(--muted);
      font: 12px/1 ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
      text-transform: uppercase;
    }
    canvas {
      width: min(28vw, 272px);
      height: min(66vh, 640px);
      image-rendering: pixelated;
      border: 1px solid #566469;
      background: var(--paper);
    }
    .logs {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 0;
      background: #080b0c;
      overflow: hidden;
    }
    pre {
      margin: 0;
      padding: 12px;
      overflow: auto;
      color: #d7e2dd;
      font: 12px/1.35 ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
      border-left: 1px solid var(--line);
      white-space: pre-wrap;
    }
    pre:first-child { border-left: 0; }
    .warn { color: var(--warn); }
    .bad { color: var(--danger); }
    @media (max-width: 980px) {
      main { grid-template-columns: 1fr; }
      .canvas-wrap { grid-template-columns: 1fr; }
      canvas { width: min(72vw, 272px); height: min(78vh, 640px); }
      .logs { grid-template-columns: 1fr; }
    }
  </style>
</head>
<body>
  <main>
    <section class="workspace">
      <div class="panel statusbar">
        <span>Eyelash Sofle Display Simulator</span>
        <span id="bridgeStatus">local serial: scanning</span>
        <span id="renderStatus">canvas renderer: running</span>
      </div>
      <div class="panel canvas-wrap">
        <div class="display">
          <div class="display-title" id="leftTitle">left</div>
          <canvas id="leftCanvas" width="68" height="160"></canvas>
        </div>
        <div class="display">
          <div class="display-title" id="rightTitle">right</div>
          <canvas id="rightCanvas" width="68" height="160"></canvas>
        </div>
      </div>
      <div class="panel logs">
        <pre id="stateLog"></pre>
        <pre id="serialLog"></pre>
      </div>
    </section>
  </main>
  <script>
    const W = 68;
    const H = 160;
    const STATUS_H = 14;
    const ANIM_Y = STATUS_H;
    const ANIM_H = H - STATUS_H;
    const SLOT_TOP = 3;
    const EDGE = 4;
    const RENDER_FPS = 12;

    const defaults = {
      left: { side: "left", role: "central", battery: "51_100_charging", activity: "idle", transport: "usb", split: "unknown", layer: "type" },
      right: { side: "right", role: "peripheral", battery: "51_100", activity: "idle", transport: "unknown", split: "unknown", layer: "type" },
    };

    const state = structuredClone(defaults);
    const theme = {
      left: newTheme("left"),
      right: newTheme("right"),
    };
    const logs = [];
    let bridgeCursor = 0;
    let bridgeActive = true;
    const parseStats = {
      lines: 0,
      snapshots: 0,
    };
    let lastFrame = 0;
    let lastKeyboardStateAt = 0;

    function newTheme(side) {
      return {
        side,
        variant: side === "right" ? "secondary" : "primary",
        scene: "normal",
        previousScene: "normal",
        phase: "idle",
        energy: "unknown",
        charging: false,
        frameTick: 0,
        typingTicks: 0,
        decayTicks: 0,
        wantsNextFrame: false,
        nextDelayMs: 0,
      };
    }

    function batteryEnergy(battery) {
      if (battery.startsWith("0_10")) return "low";
      if (battery.startsWith("11_50")) return "medium";
      if (battery.startsWith("51_100")) return "high";
      return "unknown";
    }
    function batteryCharging(battery) { return battery.endsWith("_charging"); }

    function rect(ctx, x, y, w, h, filled = true) {
      if (filled) ctx.fillRect(x, y, w, h);
      else ctx.strokeRect(x + 0.5, y + 0.5, Math.max(0, w - 1), Math.max(0, h - 1));
    }
    function centered(origin, parent, child) { return child >= parent ? origin : origin + Math.floor((parent - child) / 2); }

    function slash(ctx, b) {
      const steps = Math.min(b.w, b.h);
      for (let i = 0; i < steps; i += 2) rect(ctx, b.x + b.w - 1 - i, b.y + i, 1, 2);
    }

    function glyph(ctx, b, rows) {
      const x = centered(b.x, b.w, 5);
      const y = centered(b.y, b.h, 7);
      rows.forEach((row, yy) => {
        for (let col = 0; col < 5; col++) {
          if (row & (1 << (4 - col))) rect(ctx, x + col, y + yy, 1, 1);
        }
      });
    }

    function statusValueBattery(battery) {
      const widths = { "0_10": 4, "0_10_charging": 4, "11_50": 8, "11_50_charging": 8, "51_100": 12, "51_100_charging": 12 };
      return widths[battery] || 0;
    }

    function drawBattery(ctx, b, battery) {
      rect(ctx, b.x, b.y, b.w, b.h, false);
      const fill = statusValueBattery(battery);
      if (fill > 0) rect(ctx, b.x + 2, b.y + 2, fill, b.h - 4);
      rect(ctx, b.x + b.w - 1, b.y + 2, 1, b.h - 4);
      if (batteryCharging(battery)) rect(ctx, b.x + b.w - 6, b.y + 1, 2, b.h - 2);
      if (battery === "unknown") slash(ctx, b);
    }

    function drawSplit(ctx, b, split) {
      rect(ctx, b.x, b.y, b.w, b.h, false);
      rect(ctx, b.x + 5, b.y + b.h - 2, 2, 2);
      rect(ctx, b.x + 3, b.y + 3, 6, 1);
      rect(ctx, b.x + 1, b.y + 1, 10, 1);
      if (split !== "connected") slash(ctx, b);
    }

    function drawTransport(ctx, b, transport) {
      rect(ctx, b.x, b.y, b.w, b.h, false);
      if (transport === "usb") {
        rect(ctx, b.x + 5, b.y + 1, 2, b.h - 2);
        rect(ctx, b.x + 2, b.y + 3, b.w - 4, 1);
      } else if (transport === "bt") {
        rect(ctx, b.x + 5, b.y + 1, 1, b.h - 2);
        rect(ctx, b.x + 4, b.y + 1, 4, 1);
        rect(ctx, b.x + 4, b.y + b.h - 2, 4, 1);
      } else if (transport === "disconnected") {
        rect(ctx, b.x + 3, b.y + 3, b.w - 6, 2);
      } else {
        slash(ctx, b);
      }
    }

    function drawLayer(ctx, b, layer) {
      const rows = {
        type: [0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04],
        symbol: [0x1f, 0x10, 0x10, 0x1f, 0x01, 0x01, 0x1f],
        mod: [0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11],
        config: [0x0f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0f],
      };
      if (rows[layer]) glyph(ctx, b, rows[layer]);
      else { rect(ctx, b.x, b.y, b.w, b.h, false); slash(ctx, b); }
    }

    function drawStatus(ctx, side) {
      const s = state[side];
      rect(ctx, 0, 0, W, STATUS_H, false);
      drawBattery(ctx, { x: EDGE, y: SLOT_TOP, w: 18, h: 8 }, s.battery);
      drawSplit(ctx, { x: centered(0, W, 12), y: SLOT_TOP, w: 12, h: 8 }, s.split);
      const trailing = { x: W - EDGE - 12, y: SLOT_TOP, w: 12, h: 8 };
      if (side === "left") drawTransport(ctx, trailing, s.transport);
      else drawLayer(ctx, trailing, s.layer);
      rect(ctx, 0, STATUS_H - 1, W, 1);
    }

    function energyIntensity(energy) { return energy === "low" ? 1 : energy === "medium" ? 2 : energy === "high" ? 3 : 0; }
    function phaseIntensity(phase) {
      return phase === "typing-light" ? 1 : phase === "typing-medium" ? 2 :
        phase === "typing-high" ? 3 : phase === "typing-peak" ? 4 : phase === "decay" ? 1 : 0;
    }

    function starfield(ctx, t, energy) {
      const stars = 7 + energyIntensity(energy) * 3;
      for (let i = 0; i < stars; i++) {
        const x = (i * 17 + t.frameTick * 3) % W;
        const y = ANIM_Y + ((i * 23 + t.frameTick * 5) % ANIM_H);
        rect(ctx, x, y, 1, 1);
      }
    }

    function layerModifier(ctx, layer) {
      if (layer === "symbol") {
        for (let y = 12; y < ANIM_H; y += 18) rect(ctx, 6, ANIM_Y + y, W - 12, 1);
      } else if (layer === "mod") {
        for (let i = 0; i < 4; i++) rect(ctx, 8 + i * 12, ANIM_Y + 24 + (i % 2) * 28, 5, 5, false);
      } else if (layer === "config") {
        for (let i = 0; i < W; i += 6) rect(ctx, i, ANIM_Y + i / 2, 1, ANIM_H - i);
      }
    }

    function primary(ctx, t, energy) {
      const intensity = phaseIntensity(t.phase);
      const e = energyIntensity(energy);
      const body = 7 + intensity + e;
      const x = centered(0, W, body);
      const y = ANIM_Y + 42 + ((t.frameTick * 5) % 22);
      for (let i = 0; i < intensity + e + 1; i++) {
        rect(ctx, x > i * 3 ? x - i * 3 : 0, y > i * 5 ? y - i * 5 : ANIM_Y, Math.max(1, body - i), 2);
      }
      rect(ctx, x, y, body, body, false);
      if (t.phase === "decay") rect(ctx, centered(0, W, 28), y + body + 8, 28, 1);
    }

    function secondary(ctx, t, energy) {
      const intensity = phaseIntensity(t.phase);
      const e = energyIntensity(energy);
      const horizonY = ANIM_Y + ANIM_H - 26 - e * 3;
      rect(ctx, 6, horizonY, W - 12, 3 + e);
      const planet = 18 + e * 4 + intensity;
      rect(ctx, centered(0, W, planet), horizonY - planet + 4, planet, planet, false);
      if (intensity > 0) rect(ctx, centered(0, W, 8 + intensity * 4), ANIM_Y + 18 + ((t.frameTick * 4) % 30), 8 + intensity * 4, 2);
    }

    function drawScene(ctx, side) {
      const s = state[side];
      const t = theme[side];
      const energy = t.energy || batteryEnergy(s.battery);
      const charging = t.charging;
      rect(ctx, 0, ANIM_Y, W, ANIM_H, false);
      if (t.scene === "sleep") {
        rect(ctx, 0, ANIM_Y, W, ANIM_H);
        return;
      }
      if (t.scene === "link-error") {
        for (let y = 0; y < ANIM_H; y += 4) {
          const offset = (y / 4) % 2 === 0 ? 0 : 2;
          for (let x = offset; x < W; x += 4) rect(ctx, x, ANIM_Y + y, 1, 1);
        }
        rect(ctx, Math.floor((W - 3) / 2), ANIM_Y + Math.floor((ANIM_H - 16) / 2), 3, 10);
        rect(ctx, Math.floor((W - 3) / 2), ANIM_Y + Math.floor((ANIM_H - 16) / 2) + 13, 3, 3);
        return;
      }
      starfield(ctx, t, energy);
      layerModifier(ctx, s.layer);
      if (side === "right") secondary(ctx, t, energy);
      else primary(ctx, t, energy);
      if (charging) {
        const x = W - 8;
        const y = ANIM_Y + 4;
        rect(ctx, x + 2, y, 2, 4);
        rect(ctx, x, y + 4, 4, 1);
        rect(ctx, x, y + 5, 2, 4);
      }
      const intensity = phaseIntensity(t.phase);
      for (let i = 0; i < intensity; i++) rect(ctx, 3 + i * 4, ANIM_Y + 4, 2, 2);
    }

    function renderDisplay(side) {
      const canvas = document.getElementById(`${side}Canvas`);
      const ctx = canvas.getContext("2d");
      ctx.fillStyle = "#eef5ef";
      ctx.fillRect(0, 0, W, H);
      ctx.fillStyle = "#111";
      ctx.strokeStyle = "#111";
      ctx.lineWidth = 1;
      drawStatus(ctx, side);
      drawScene(ctx, side);
      document.getElementById(`${side}Title`).textContent =
        `${side} ${theme[side].phase} tick=${theme[side].frameTick} split=${state[side].split}`;
    }

    function renderLoop(now) {
      const fps = RENDER_FPS;
      if (now - lastFrame >= 1000 / fps) {
        renderDisplay("left");
        renderDisplay("right");
        updateStateLog();
        lastFrame = now;
      }
      requestAnimationFrame(renderLoop);
    }

    function applyLine(line, source = "") {
      parseStats.lines += 1;
      logs.push(source ? `[${source}] ${line}` : line);
      if (logs.length > 300) logs.shift();
      document.getElementById("serialLog").textContent = logs.slice(-120).join("\\n");
    }

    function applySnapshot(snapshot) {
      if (!snapshot || !snapshot.left || !snapshot.right) return;
      for (const side of ["left", "right"]) {
        Object.assign(state[side], snapshot[side].state || {});
        Object.assign(theme[side], snapshot[side].theme || {});
      }
      parseStats.snapshots += 1;
      lastKeyboardStateAt = performance.now();
    }

    async function pollLocalSerial() {
      if (!bridgeActive) return;
      try {
        const response = await fetch(`/api/serial?since=${bridgeCursor}`, { cache: "no-store" });
        const body = await response.json();
        bridgeCursor = body.cursor || bridgeCursor;
        for (const entry of body.entries || []) {
          applyLine(String(entry.line || "").replace(/\\x1b\\[[0-9;]*m/g, ""), entry.source || "local serial");
        }
        applySnapshot(body.snapshot);
        const portCount = (body.ports || []).length;
        const errorCount = Object.keys(body.errors || {}).length;
        document.getElementById("bridgeStatus").textContent =
          `local serial: ${portCount} port(s), ${errorCount} error(s)`;
      } catch (err) {
        document.getElementById("bridgeStatus").innerHTML =
          `<span class="bad">local serial: ${err.message}</span>`;
      } finally {
        setTimeout(pollLocalSerial, 250);
      }
    }

    function updateStateLog() {
      const now = performance.now();
      const keyboardFresh = lastKeyboardStateAt && now - lastKeyboardStateAt < 3000;
      document.getElementById("renderStatus").textContent =
        `canvas renderer: running, controller: ${keyboardFresh ? "keyboard logs" : "waiting for keyboard logs"}`;
      document.getElementById("stateLog").textContent = JSON.stringify({
        controller: "keyboard logs",
        lastKeyboardStateMsAgo: lastKeyboardStateAt ? Math.round(now - lastKeyboardStateAt) : null,
        serialParse: parseStats,
        state,
        theme,
      }, null, 2);
    }

    updateStateLog();
    pollLocalSerial();
    requestAnimationFrame(renderLoop);
  </script>
</body>
</html>
"""


class Handler(BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/api/serial":
            query = parse_qs(parsed.query)
            try:
                since = int(query.get("since", ["0"])[0])
            except ValueError:
                since = 0
            payload = SERIAL_TAILER.snapshot(since)
            payload["snapshot"] = KEYBOARD_CONTROLLER.apply_entries(payload["entries"], payload["ports"])
            body = json.dumps(payload).encode("utf-8")
            self.send_response(200)
            self.send_header("content-type", "application/json")
            self.send_header("content-length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
            return

        if parsed.path != "/":
            self.send_error(404)
            return

        body = PAGE.encode("utf-8")
        self.send_response(200)
        self.send_header("content-type", "text/html; charset=utf-8")
        self.send_header("content-length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt: str, *args: object) -> None:
        return


def main() -> None:
    ENGINE.start()
    SERIAL_TAILER.start()
    server = ThreadingHTTPServer(("0.0.0.0", PORT), Handler)
    print(f"dual display canvas simulator listening on http://localhost:{PORT}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
