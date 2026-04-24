# Commands

## Cheap Validation

```bash
make verify
```

## Refresh West Workspace

```bash
cd .zmk
west update
```

## Local Firmware Builds

Run these from the repo root:

```bash
REPO_ROOT=$(pwd)
cd .zmk

west build -d build/left -s zmk/app -b eyelash_sofle_left -- \
  -DZMK_CONFIG="$REPO_ROOT/config" \
  -DSHIELD=nice_view

west build -d build/right -s zmk/app -b eyelash_sofle_right -- \
  -DZMK_CONFIG="$REPO_ROOT/config" \
  -DSHIELD=nice_view
```

## Studio Build

```bash
REPO_ROOT=$(pwd)
cd .zmk

west build -d build/left-studio -s zmk/app -b eyelash_sofle_left -S studio-rpc-usb-uart -- \
  -DZMK_CONFIG="$REPO_ROOT/config" \
  -DSHIELD=nice_view \
  -DCONFIG_ZMK_STUDIO=y \
  -DCONFIG_ZMK_STUDIO_LOCKING=n
```

## Settings Reset Build

```bash
REPO_ROOT=$(pwd)
cd .zmk

west build -d build/left-settings-reset -s zmk/app -b eyelash_sofle_left -- \
  -DZMK_CONFIG="$REPO_ROOT/config" \
  -DSHIELD=settings_reset

west build -d build/right-settings-reset -s zmk/app -b eyelash_sofle_right -- \
  -DZMK_CONFIG="$REPO_ROOT/config" \
  -DSHIELD=settings_reset
```

## USB Logging Debug Builds

```bash
REPO_ROOT=$(pwd)
cd .zmk

west build -d build/left-studio-debug -s zmk/app -b eyelash_sofle_left -S studio-rpc-usb-uart -- \
  -DZMK_CONFIG="$REPO_ROOT/config" \
  -DSHIELD=nice_view \
  -DCONFIG_ZMK_USB_LOGGING=y \
  -DCONFIG_ZMK_STUDIO=y \
  -DCONFIG_ZMK_STUDIO_LOCKING=n \
  -DCONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS=8000

west build -d build/right-debug -s zmk/app -b eyelash_sofle_right -S zmk-usb-logging -- \
  -DZMK_CONFIG="$REPO_ROOT/config" \
  -DSHIELD=nice_view \
  -DCONFIG_ZMK_USB=y \
  -DCONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS=8000
```

## Flash And Debug Notes

- Flash the left and right firmware to their matching halves.
- For split pairing issues, flash both `eyelash_sofle_left_settings_reset` and `eyelash_sofle_right_settings_reset`, then flash normal firmware again. See `.agentic/troubleshooting/split-pairing.md`.
- For runtime logs, flash the relevant `*_debug` artifact and open its USB CDC ACM serial device, for example:

```bash
sudo tio /dev/ttyACM0
```

- Do not expect peripheral logs to relay through the central. Debug each half over its own USB connection.
