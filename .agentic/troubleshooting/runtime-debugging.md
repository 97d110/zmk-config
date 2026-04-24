# Runtime Debugging

Use this guide when both halves are flashed with the dedicated debug artifacts:

- `eyelash_sofle_left_studio_debug`
- `eyelash_sofle_right_debug`

Debug firmware exposes USB CDC ACM logs from each half. Do not expect right-half logs to relay through the left central; plug into and read each half directly.

## Find The Log Ports

List attached ACM devices and stable symlinks:

```bash
ls -l /dev/ttyACM*
ls -l /dev/serial/by-id
```

Typical debug sessions expose two ACM interfaces per board. In one observed session:

- left central logs: `/dev/ttyACM0`
- right peripheral logs: `/dev/ttyACM3`
- the other two ACM interfaces were quiet

If `tio` is installed, read a port interactively:

```bash
sudo tio /dev/ttyACM0
```

For a short non-interactive capture:

```bash
mkdir -p .tmp
timeout 12s cat /dev/ttyACM0 > .tmp/left-central.log
timeout 12s cat /dev/ttyACM3 > .tmp/right-peripheral.log
```

Raw CDC reads can start mid-line or contain torn log lines. Capture again if a specific event needs clean context.

## Healthy Split Input Path

For right-half keys, expect this sequence:

1. Right peripheral logs a scan event:

```text
kscan_matrix_read: Sending event at <row>,<col> state on
zmk_physical_layouts_kscan_process_msgq: Row: <row>, col: <col>, position: <position>, pressed: true
split_peripheral_listener:
```

2. Left central receives the split notification:

```text
split_central_notify_func: [NOTIFICATION]
peripheral_event_work_callback: Trigger key position state change for <position>
```

3. Left central applies the keymap and emits HID:

```text
zmk_keymap_apply_position_state: layer_id: <layer> position: <position>, binding name: <behavior>
hid_listener_keycode_pressed
zmk_endpoints_send_report
```

If all three stages appear, the right half is scanning, split transport is working, and the central is producing host reports.

## Scenarios

Right keys scan but do not type:

- Right log has `kscan_matrix_read` and `split_peripheral_listener`.
- Left log does not show `split_central_notify_func` for the same presses.
- Suspect split pairing or BLE transport state. Follow `.agentic/troubleshooting/split-pairing.md`, including settings reset on both halves.

Right keys reach central but do not type:

- Left log has `split_central_notify_func` and `zmk_keymap_apply_position_state`.
- Left log lacks `zmk_endpoints_send_report`, or endpoint logs show the wrong selected output.
- Check output selection, USB/BLE readiness, and the active layer binding for the reported position.

Left keys do not type:

- Left log should show local `kscan_matrix_read`, `zmk_keymap_apply_position_state`, `hid_listener_keycode_pressed`, and `zmk_endpoints_send_report`.
- If scan logs are missing, inspect matrix GPIO, transform, or hardware for the left half.
- If scan logs exist but HID logs are missing, inspect the keymap binding and active layer.

Output switching confusion:

- Layer 2 contains output controls in `config/eyelash_sofle.keymap`.
- `&out OUT_USB` and `&out OUT_BLE` should produce `zmk_endpoints_select_transport` logs.
- If only USB is ready, BLE selection may log that USB is the only ready transport.
- `&out` can log a behavior error on key release; evaluate whether the press selected the intended transport before treating release-only errors as fatal.

Battery and power checks:

- Look for `vddh_sample_fetch` lines.
- Healthy examples show ADC raw values and a computed millivolt/percentage reading.
- Missing battery logs alone is not enough to diagnose input issues; correlate with scan and HID logs.

No logs from a port:

- Try the paired ACM interface for the same board.
- Confirm the board is flashed with a debug artifact, not normal firmware.
- Replug the board and recheck `/dev/serial/by-id`.
- Confirm local permissions; Linux usually requires `dialout` membership or `sudo` for `/dev/ttyACM*`.

## What To Save

For handoff, save one combined capture under `.tmp/` with:

- timestamp
- port-to-half mapping
- left central log window
- right peripheral log window
- the exact key sequence pressed during the capture

Keep `.tmp/*.log` as local diagnostic artifacts. Do not commit raw logs unless they are intentionally needed for a bug report.
