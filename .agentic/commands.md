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

## Firmware Builds

Firmware artifacts are built by GitHub Actions from commits using the matrix in
`build.yaml`. Do not run local `west update` or `west build` for routine agent
validation unless the user explicitly asks for a local build.

## Flash And Debug Notes

- Flash the left and right firmware to their matching halves.
- For split pairing issues, flash both `eyelash_sofle_left_settings_reset` and `eyelash_sofle_right_settings_reset`, then flash normal firmware again. See `.agentic/troubleshooting/split-pairing.md`.
- For runtime logs, flash the relevant `*_debug` artifact and open its USB CDC ACM serial device, for example:

```bash
sudo tio /dev/ttyACM0
```

- Do not expect peripheral logs to relay through the central. Debug each half over its own USB connection.
