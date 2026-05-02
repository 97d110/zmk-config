#!/usr/bin/env python3
#
# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

from __future__ import annotations

import json
import os
import subprocess
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlparse


ROOT_DIR = Path(__file__).resolve().parents[2]
SIM_BIN = ROOT_DIR / "sim" / "build" / "dual_display_sim"
PORT = int(os.environ.get("PORT", "8080"))


DEFAULT_STATE = {
    "left": {
        "battery": 100,
        "charging": False,
        "activity": 0,
        "sleep": False,
        "split": "unknown",
        "transport": "unknown",
        "layer": 0,
    },
    "right": {
        "battery": 100,
        "charging": False,
        "activity": 0,
        "sleep": False,
        "split": "unknown",
        "transport": "unknown",
        "layer": 0,
    },
}

STATE = json.loads(json.dumps(DEFAULT_STATE))


def build_simulator() -> None:
    subprocess.run(["make", "-C", str(ROOT_DIR / "sim"), "all"], check=True)


def command_script(state: dict) -> str:
    commands: list[str] = []
    for side in ("left", "right"):
        side_state = state[side]
        charging = " charging" if side_state["charging"] else ""
        commands.append(f"battery {side} {int(side_state['battery'])}{charging}")
        commands.append(f"transport {side} {side_state['transport']}")
        commands.append(f"split {side} {side_state['split']}")
        commands.append(f"layer {side} {int(side_state['layer'])}")
        if side_state["sleep"]:
            commands.append(f"sleep {side} on")
        else:
            commands.append(f"activity {side} {int(side_state['activity'])}")
    commands.append("show")
    commands.append("quit")
    return "\n".join(commands) + "\n"


def run_simulator(state: dict) -> dict:
    build_simulator()
    result = subprocess.run(
        [str(SIM_BIN), "--batch"],
        input=command_script(state),
        text=True,
        capture_output=True,
        check=False,
    )
    return {
        "stdout": result.stdout,
        "stderr": result.stderr,
        "returncode": result.returncode,
    }


def clamp_int(value: object, minimum: int, maximum: int) -> int:
    try:
        parsed = int(value)
    except (TypeError, ValueError):
        parsed = minimum
    return max(minimum, min(maximum, parsed))


def normalize_side_state(raw: dict, fallback: dict) -> dict:
    transport = raw.get("transport", fallback["transport"])
    if transport not in {"unknown", "usb", "bt", "disconnected"}:
        transport = "unknown"

    split = raw.get("split", fallback["split"])
    if split not in {"unknown", "connected", "disconnected"}:
        split = "unknown"

    return {
        "battery": clamp_int(raw.get("battery", fallback["battery"]), 0, 100),
        "charging": bool(raw.get("charging", fallback["charging"])),
        "activity": clamp_int(raw.get("activity", fallback["activity"]), 0, 15000),
        "sleep": bool(raw.get("sleep", fallback["sleep"])),
        "split": split,
        "transport": transport,
        "layer": clamp_int(raw.get("layer", fallback["layer"]), 0, 3),
    }


def update_state(payload: dict) -> dict:
    global STATE

    next_state = json.loads(json.dumps(STATE))
    for side in ("left", "right"):
        if isinstance(payload.get(side), dict):
            next_state[side] = normalize_side_state(payload[side], STATE[side])
    STATE = next_state
    return STATE


def page_html() -> str:
    initial = json.dumps(STATE).replace("<", "\\u003c")
    return f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Dual Display Simulator</title>
  <style>
    :root {{
      color-scheme: dark;
      --bg: #101416;
      --panel: #171d20;
      --panel-2: #20282c;
      --text: #ecf2f0;
      --muted: #9fb0ab;
      --line: #38464b;
      --accent: #65d6ad;
      --warn: #f2c15f;
      --danger: #ff7a72;
    }}
    * {{ box-sizing: border-box; }}
    body {{
      margin: 0;
      min-height: 100vh;
      background: var(--bg);
      color: var(--text);
      font: 14px/1.45 system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    }}
    main {{
      display: grid;
      grid-template-columns: minmax(280px, 360px) minmax(520px, 1fr);
      gap: 18px;
      min-height: 100vh;
      padding: 18px;
    }}
    aside, section {{
      min-width: 0;
    }}
    h1 {{
      margin: 0 0 14px;
      font-size: 18px;
      font-weight: 700;
      letter-spacing: 0;
    }}
    h2 {{
      margin: 16px 0 10px;
      color: var(--muted);
      font-size: 12px;
      font-weight: 700;
      letter-spacing: 0;
      text-transform: uppercase;
    }}
    .controls {{
      border: 1px solid var(--line);
      background: var(--panel);
      padding: 14px;
    }}
    .side {{
      display: grid;
      gap: 10px;
      padding: 12px 0;
      border-top: 1px solid var(--line);
    }}
    .side:first-of-type {{ border-top: 0; padding-top: 0; }}
    label {{
      display: grid;
      gap: 5px;
      color: var(--muted);
      font-size: 12px;
      font-weight: 600;
    }}
    input, select, button {{
      width: 100%;
      border: 1px solid var(--line);
      background: var(--panel-2);
      color: var(--text);
      padding: 8px 9px;
      font: inherit;
    }}
    input[type="checkbox"] {{
      width: auto;
      margin: 0;
    }}
    .inline {{
      display: flex;
      align-items: center;
      gap: 8px;
      color: var(--text);
    }}
    .grid2 {{
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
    }}
    .actions {{
      display: flex;
      gap: 10px;
      margin-top: 14px;
    }}
    button {{
      cursor: pointer;
      font-weight: 700;
    }}
    button.primary {{
      border-color: #2f735e;
      background: #1f5f4a;
    }}
    .workspace {{
      display: grid;
      grid-template-rows: auto minmax(340px, 1fr) minmax(180px, 36vh);
      gap: 14px;
      min-height: calc(100vh - 36px);
    }}
    .statusbar {{
      display: flex;
      justify-content: space-between;
      gap: 14px;
      border: 1px solid var(--line);
      background: var(--panel);
      padding: 10px 12px;
      color: var(--muted);
      font-family: ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
      overflow: auto;
    }}
    .preview, .logs {{
      border: 1px solid var(--line);
      background: #0b0f10;
      overflow: auto;
    }}
    pre {{
      margin: 0;
      padding: 14px;
      color: #d9f4e7;
      font: 13px/1.25 ui-monospace, SFMono-Regular, Menlo, Consolas, monospace;
      white-space: pre;
    }}
    .logs pre {{ color: #d6dde0; }}
    .error {{ color: var(--danger); }}
    @media (max-width: 900px) {{
      main {{ grid-template-columns: 1fr; }}
      .workspace {{ min-height: auto; }}
    }}
  </style>
</head>
<body>
  <main>
    <aside class="controls">
      <h1>Dual Display Simulator</h1>
      <form id="controls"></form>
      <div class="actions">
        <button class="primary" id="apply" type="button">Render</button>
        <button id="reset" type="button">Reset</button>
      </div>
    </aside>
    <section class="workspace">
      <div class="statusbar">
        <span id="result">ready</span>
        <span>shared core via sim/build/dual_display_sim</span>
      </div>
      <div class="preview"><pre id="preview"></pre></div>
      <div class="logs"><pre id="logs"></pre></div>
    </section>
  </main>
  <script>
    const initialState = {initial};
    let state = structuredClone(initialState);
    const sides = ["left", "right"];

    function sideControls(side) {{
      const s = state[side];
      return `
        <div class="side">
          <h2>${{side}}</h2>
          <div class="grid2">
            <label>Battery
              <input data-side="${{side}}" data-key="battery" type="number" min="0" max="100" value="${{s.battery}}">
            </label>
            <label>Activity ms
              <input data-side="${{side}}" data-key="activity" type="number" min="0" max="15000" step="250" value="${{s.activity}}">
            </label>
          </div>
          <div class="grid2">
            <label>Transport
              <select data-side="${{side}}" data-key="transport">
                ${{["unknown", "usb", "bt", "disconnected"].map(v => `<option value="${{v}}" ${{s.transport === v ? "selected" : ""}}>${{v}}</option>`).join("")}}
              </select>
            </label>
            <label>Split
              <select data-side="${{side}}" data-key="split">
                ${{["unknown", "connected", "disconnected"].map(v => `<option value="${{v}}" ${{s.split === v ? "selected" : ""}}>${{v}}</option>`).join("")}}
              </select>
            </label>
          </div>
          <label>Layer
            <select data-side="${{side}}" data-key="layer">
              ${{[0, 1, 2, 3].map(v => `<option value="${{v}}" ${{s.layer === v ? "selected" : ""}}>${{v}}</option>`).join("")}}
            </select>
          </label>
          <label class="inline">
            <input data-side="${{side}}" data-key="charging" type="checkbox" ${{s.charging ? "checked" : ""}}>
            charging
          </label>
          <label class="inline">
            <input data-side="${{side}}" data-key="sleep" type="checkbox" ${{s.sleep ? "checked" : ""}}>
            sleep
          </label>
        </div>`;
    }}

    function renderControls() {{
      document.querySelector("#controls").innerHTML = sides.map(sideControls).join("");
      document.querySelectorAll("[data-side]").forEach(input => {{
        input.addEventListener("input", () => {{
          const side = input.dataset.side;
          const key = input.dataset.key;
          if (input.type === "checkbox") {{
            state[side][key] = input.checked;
          }} else if (input.type === "number" || key === "layer") {{
            state[side][key] = Number(input.value);
          }} else {{
            state[side][key] = input.value;
          }}
        }});
      }});
    }}

    async function render() {{
      document.querySelector("#result").textContent = "rendering";
      const response = await fetch("/api/render", {{
        method: "POST",
        headers: {{ "content-type": "application/json" }},
        body: JSON.stringify(state),
      }});
      const body = await response.json();
      state = body.state;
      document.querySelector("#preview").textContent = body.stdout || "";
      document.querySelector("#logs").textContent = body.stderr || "";
      document.querySelector("#result").textContent = body.returncode === 0 ? "ok" : `exit ${{body.returncode}}`;
      document.querySelector("#result").className = body.returncode === 0 ? "" : "error";
      renderControls();
    }}

    document.querySelector("#apply").addEventListener("click", render);
    document.querySelector("#reset").addEventListener("click", () => {{
      state = structuredClone(initialState);
      renderControls();
      render();
    }});
    renderControls();
    render();
  </script>
</body>
</html>"""


class Handler(BaseHTTPRequestHandler):
    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path != "/":
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header("content-type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(page_html().encode("utf-8"))

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path != "/api/render":
            self.send_error(404)
            return
        length = int(self.headers.get("content-length", "0"))
        payload = json.loads(self.rfile.read(length) or b"{}")
        state = update_state(payload if isinstance(payload, dict) else {})
        result = run_simulator(state)
        result["state"] = state
        response = json.dumps(result).encode("utf-8")
        self.send_response(200)
        self.send_header("content-type", "application/json")
        self.send_header("content-length", str(len(response)))
        self.end_headers()
        self.wfile.write(response)

    def log_message(self, fmt: str, *args: object) -> None:
        return


def main() -> None:
    build_simulator()
    server = ThreadingHTTPServer(("0.0.0.0", PORT), Handler)
    print(f"dual display web simulator listening on http://0.0.0.0:{PORT}", flush=True)
    server.serve_forever()


if __name__ == "__main__":
    main()
