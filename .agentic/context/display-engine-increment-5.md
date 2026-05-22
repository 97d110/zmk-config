# Display Engine Increment 5 Handoff

Increment 5 wires the local dual-display state model to real ZMK firmware
state and events. The browser simulator now treats those firmware logs as the
primary controller, while firmware initializes from and reacts to runtime
sources.

## Implemented

- Added `display/firmware/dual_display_state_adapter.c`, a durable Zephyr-only
  state adapter that initializes display state from ZMK runtime APIs.
- Subscribed the adapter to activity, battery, USB, endpoint, BLE profile,
  keycode, layer, and split-peripheral events behind the same configuration
  and role guards used by ZMK's own widgets.
- Added `zmk_dual_display_status_screen_update_from_state()` so event-driven
  refreshes reuse the existing LVGL screen lifecycle and renderer contract.
- Kept keymap and endpoint calls central-only so right-side peripheral builds
  do not link against central HID/keymap objects.
- Added trace logs for initial firmware mapping, event application, keypress
  streak mapping, no-op updates, skipped pre-init events, and render queueing.
- Cross-checked the adjacent `nice-view-gem`, `mario-peripheral-animation`,
  `zmk-nice-oled`, and `zmk-split-peripheral-output-relay` repos as read-only
  references. Their central/peripheral display split and event subscriptions
  informed the role guards here.

## Behavior Notes

- Battery buckets come from `zmk_battery_state_of_charge()` and use USB power
  presence as the charging signal when the USB device stack is enabled.
- Initial activity maps ZMK active/idle/sleep to semantic typing, idle, and
  sleep display states. Runtime idle/sleep events update display state
  directly, while runtime active events leave typing state to the central-side
  keypress cycle. Keypress events only set a lightweight boolean for the
  current typing period. A configurable delayed work cycle logs the complete
  display state, keeps activity at typing while keypresses continue, and
  returns activity to idle after a quiet period.
- Central firmware maps the highest active keymap layer and selected endpoint
  into display layer and transport state. BLE transport shows connected only
  when the active BLE profile is connected.
- Split link state is initialized from the peripheral-side split Bluetooth
  status where that API exists, and otherwise remains unknown until a split
  event is available.

## Validation

- Run `make sim` or `make sim-web` to inspect the browser canvas simulator.
- Run `make verify` after this increment; the verifier now checks that the
  firmware adapter, subscriptions, refresh entry point, and handoff note are
  present.

Firmware build validation is still handled by GitHub Actions from commits.
Never plan, suggest, or attempt local `west update` or `west build`.
