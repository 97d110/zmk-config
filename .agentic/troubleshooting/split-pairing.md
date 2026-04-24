# Split Pairing Troubleshooting

## Symptom

The right half appears connected to the left half, but right-side key input does not reach the left central or the host machine.

## Most Likely Cause

For BLE split keyboards, pairing and bond state is persisted on both halves. Flashing normal firmware does not clear that state. Resetting only the left central can leave the right peripheral holding stale split bond information.

This repo must keep settings-reset artifacts for both halves:

- `eyelash_sofle_left_settings_reset`
- `eyelash_sofle_right_settings_reset`

## Recovery Sequence

1. Flash `eyelash_sofle_left_settings_reset` to the left half.
2. Flash `eyelash_sofle_right_settings_reset` to the right half.
3. Boot each reset firmware once.
4. Flash the normal or Studio firmware back to the left half.
5. Flash the normal right firmware back to the right half.
6. Power-cycle or reset both halves at roughly the same time so they can re-pair.
7. If using USB output, confirm the left central is selected for USB output or reset persisted settings again.

## Debug Logging

Use the dedicated debug artifacts only while diagnosing runtime issues:

- `eyelash_sofle_left_studio_debug`
- `eyelash_sofle_right_debug`

Read logs from each half over its own USB CDC ACM serial device:

```bash
ls /dev/ttyACM*
sudo tio /dev/ttyACM0
```

Do not expect right-half logs to relay through the left central. Plug in and inspect each half directly.

## Build Matrix Rule

Do not remove the right settings-reset build. If split pairing breaks again, central-only reset is incomplete.
