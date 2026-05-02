# Commands

## Cheap Validation

```bash
make verify
```

If the host machine is missing validation dependencies such as `rg`, run the
same checks in Docker:

```bash
make verify-docker
```

## Display Simulator

```bash
make sim
```

The simulator builds the durable `display/core/` planner on the host and opens
an interactive console preview for both screens. It prints `zmk_dual_display`
diagnostics to stderr while state changes are applied.

```bash
make sim-web
make sim-web-docker
make -C sim/web docker
```

The web simulator exposes the same host simulator through a browser UI. Docker
serves it on `http://localhost:8080` by default; override the host port with
`SIM_WEB_PORT=<port>` from the repo root or `PORT=<port>` from `sim/web/`.

## Firmware Builds

Firmware artifacts are built by GitHub Actions from commits using the matrix in
`build.yaml`. Do not run local `west update` or `west build` for routine agent
validation unless the user explicitly asks for a local build.

## Flash And Debug Notes

- Flash the left and right firmware to their matching halves.
- Display-engine firmware now reacts to ZMK activity, keypress, battery, layer,
  endpoint, USB, BLE, and split-link events. Use the
  `*_display_engine_debug` artifacts when validating those runtime transitions
  over USB logs.
- For split pairing issues, flash both `eyelash_sofle_left_settings_reset` and `eyelash_sofle_right_settings_reset`, then flash normal firmware again. See `.agentic/troubleshooting/split-pairing.md`.
- For runtime logs, flash the relevant `*_debug` artifact and open its USB CDC ACM serial device, for example:

```bash
sudo tio /dev/ttyACM0
```

- Do not expect peripheral logs to relay through the central. Debug each half over its own USB connection.
