/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <display/firmware/dual_display_state_adapter.h>
#include <display/log.h>
#include <display/render/lvgl/dual_display_status_screen.h>

#include <zmk/activity.h>
#include <zmk/display.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
#include <zmk/battery.h>
#include <zmk/events/battery_state_changed.h>
#endif

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/usb.h>
#endif

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/endpoints.h>
#include <zmk/events/endpoint_changed.h>
#include <zmk/events/keycode_state_changed.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/keymap.h>

#if IS_ENABLED(CONFIG_ZMK_BLE)
#include <zmk/ble.h>
#include <zmk/events/ble_active_profile_changed.h>
#endif
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT)
#include <zmk/events/split_peripheral_status_changed.h>

#if !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
#include <zmk/split/bluetooth/peripheral.h>
#endif
#endif

static struct zmk_dual_display_state firmware_state;
static bool firmware_state_ready;
static uint32_t typing_streak_started_at;
static uint32_t last_keypress_at;

static void firmware_render_work_cb(struct k_work *work);

K_MUTEX_DEFINE(firmware_state_mutex);
K_WORK_DEFINE(firmware_render_work, firmware_render_work_cb);

#define TYPING_STREAK_RESET_MS 2000

static bool states_equal(const struct zmk_dual_display_state *left,
                         const struct zmk_dual_display_state *right) {
    return left->side == right->side && left->role == right->role &&
           left->battery == right->battery && left->activity == right->activity &&
           left->transport == right->transport && left->split_link == right->split_link &&
           left->layer == right->layer;
}

static bool current_usb_powered(void) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    return zmk_usb_is_powered();
#else
    return false;
#endif
}

static enum zmk_dual_display_activity_bucket
activity_bucket_from_zmk(enum zmk_activity_state activity) {
    switch (activity) {
    case ZMK_ACTIVITY_ACTIVE:
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped ZMK active activity to typing display bucket");
        return ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_2S;
    case ZMK_ACTIVITY_IDLE:
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped ZMK idle activity to idle display bucket");
        return ZMK_DUAL_DISPLAY_ACTIVITY_IDLE;
    case ZMK_ACTIVITY_SLEEP:
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped ZMK sleep activity to sleep display bucket");
        return ZMK_DUAL_DISPLAY_ACTIVITY_SLEEP;
    default:
        ZMK_DUAL_DISPLAY_LOG_WRN("mapped unknown ZMK activity %d to idle display bucket",
                                 activity);
        return ZMK_DUAL_DISPLAY_ACTIVITY_IDLE;
    }
}

static enum zmk_dual_display_battery_bucket current_battery_bucket(void) {
#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
    return zmk_dual_display_battery_bucket_from_percent(zmk_battery_state_of_charge(),
                                                        current_usb_powered());
#else
    ZMK_DUAL_DISPLAY_LOG_DBG("battery reporting disabled, keeping battery display unknown");
    return ZMK_DUAL_DISPLAY_BATTERY_UNKNOWN;
#endif
}

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
static enum zmk_dual_display_transport_state current_transport_state(void) {
    const struct zmk_endpoint_instance endpoint = zmk_endpoints_selected();

    switch (endpoint.transport) {
    case ZMK_TRANSPORT_USB:
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped selected ZMK USB endpoint to USB display transport");
        return ZMK_DUAL_DISPLAY_TRANSPORT_USB;
    case ZMK_TRANSPORT_BLE:
#if IS_ENABLED(CONFIG_ZMK_BLE)
        return zmk_dual_display_transport_state_from_flags(
            false, zmk_ble_active_profile_is_connected());
#else
        ZMK_DUAL_DISPLAY_LOG_DBG("BLE endpoint selected without BLE support, marking disconnected");
        return ZMK_DUAL_DISPLAY_TRANSPORT_DISCONNECTED;
#endif
    default:
        ZMK_DUAL_DISPLAY_LOG_WRN("mapped unknown ZMK endpoint transport %d to unknown display state",
                                 endpoint.transport);
        return ZMK_DUAL_DISPLAY_TRANSPORT_UNKNOWN;
    }
}

static enum zmk_dual_display_layer_mode current_layer_mode(void) {
    return zmk_dual_display_layer_mode_from_index(zmk_keymap_highest_layer_active());
}

static enum zmk_dual_display_activity_bucket typing_activity_from_keypress(uint32_t timestamp_ms) {
    if (typing_streak_started_at == 0 || timestamp_ms < last_keypress_at ||
        (timestamp_ms - last_keypress_at) > TYPING_STREAK_RESET_MS) {
        typing_streak_started_at = timestamp_ms;
        ZMK_DUAL_DISPLAY_LOG_DBG("started display typing streak at %u ms",
                                 (unsigned int)timestamp_ms);
    }

    last_keypress_at = timestamp_ms;

    return zmk_dual_display_activity_bucket_from_typing_streak(
        timestamp_ms - typing_streak_started_at + 1, false);
}
#endif

static enum zmk_dual_display_split_link_state current_split_link_state(void) {
#if IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    return zmk_split_bt_peripheral_is_connected()
               ? ZMK_DUAL_DISPLAY_SPLIT_LINK_CONNECTED
               : ZMK_DUAL_DISPLAY_SPLIT_LINK_DISCONNECTED;
#else
    ZMK_DUAL_DISPLAY_LOG_DBG("split link source unavailable on this side, keeping link unknown");
    return ZMK_DUAL_DISPLAY_SPLIT_LINK_UNKNOWN;
#endif
}

static void overlay_current_zmk_state(struct zmk_dual_display_state *state) {
    state->battery = current_battery_bucket();
    state->activity = activity_bucket_from_zmk(zmk_activity_get_state());
    state->split_link = current_split_link_state();

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    state->transport = current_transport_state();
    state->layer = current_layer_mode();
#endif
}

void zmk_dual_display_firmware_init_state(enum zmk_dual_display_side side,
                                          struct zmk_dual_display_state *out_state) {
    if (out_state == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("ignored firmware state init with NULL output buffer");
        return;
    }

    zmk_dual_display_default_state(side, out_state);
    overlay_current_zmk_state(out_state);

    k_mutex_lock(&firmware_state_mutex, K_FOREVER);
    firmware_state = *out_state;
    firmware_state_ready = true;
    k_mutex_unlock(&firmware_state_mutex);

    ZMK_DUAL_DISPLAY_LOG_DBG("initialized firmware-backed display state");
    zmk_dual_display_log_state_transition(NULL, out_state);
}

static bool apply_event_to_state(const zmk_event_t *eh, struct zmk_dual_display_state *state) {
    if (eh == NULL || state == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("ignored display state event update with missing input");
        return false;
    }

    const struct zmk_activity_state_changed *activity = as_zmk_activity_state_changed(eh);
    if (activity != NULL) {
        if (activity->state != ZMK_ACTIVITY_ACTIVE) {
            typing_streak_started_at = 0;
            last_keypress_at = 0;
        }
        state->activity = activity_bucket_from_zmk(activity->state);
        ZMK_DUAL_DISPLAY_LOG_DBG("applied activity event to display state");
        return true;
    }

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
    const struct zmk_battery_state_changed *battery = as_zmk_battery_state_changed(eh);
    if (battery != NULL) {
        state->battery = zmk_dual_display_battery_bucket_from_percent(
            battery->state_of_charge, current_usb_powered());
        ZMK_DUAL_DISPLAY_LOG_DBG("applied battery event to display state: percent=%u",
                                 (unsigned int)battery->state_of_charge);
        return true;
    }
#endif

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    const struct zmk_usb_conn_state_changed *usb = as_zmk_usb_conn_state_changed(eh);
    if (usb != NULL) {
        state->battery = current_battery_bucket();
#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
        state->transport = current_transport_state();
#endif
        ZMK_DUAL_DISPLAY_LOG_DBG("applied USB connection event to display state: conn=%d",
                                 usb->conn_state);
        return true;
    }
#endif

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    const struct zmk_layer_state_changed *layer = as_zmk_layer_state_changed(eh);
    if (layer != NULL) {
        state->layer = current_layer_mode();
        ZMK_DUAL_DISPLAY_LOG_DBG(
            "applied layer event to display state: layer=%u active=%d",
            (unsigned int)layer->layer, layer->state);
        return true;
    }

    const struct zmk_keycode_state_changed *keycode = as_zmk_keycode_state_changed(eh);
    if (keycode != NULL) {
        if (!keycode->state) {
            ZMK_DUAL_DISPLAY_LOG_DBG("ignored keycode release for display typing streak");
            return true;
        }

        const uint32_t timestamp_ms =
            keycode->timestamp > 0 ? (uint32_t)keycode->timestamp : k_uptime_get_32();
        state->activity = typing_activity_from_keypress(timestamp_ms);
        ZMK_DUAL_DISPLAY_LOG_DBG("applied keycode press to display typing streak: keycode=%u",
                                 (unsigned int)keycode->keycode);
        return true;
    }

    const struct zmk_endpoint_changed *endpoint = as_zmk_endpoint_changed(eh);
    if (endpoint != NULL) {
        state->transport = current_transport_state();
        ZMK_DUAL_DISPLAY_LOG_DBG("applied endpoint event to display state: transport=%d",
                                 endpoint->endpoint.transport);
        return true;
    }

#if IS_ENABLED(CONFIG_ZMK_BLE)
    const struct zmk_ble_active_profile_changed *ble = as_zmk_ble_active_profile_changed(eh);
    if (ble != NULL) {
        state->transport = current_transport_state();
        ZMK_DUAL_DISPLAY_LOG_DBG("applied BLE profile event to display state: profile=%u",
                                 (unsigned int)ble->index);
        return true;
    }
#endif
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT)
    const struct zmk_split_peripheral_status_changed *split =
        as_zmk_split_peripheral_status_changed(eh);
    if (split != NULL) {
        state->split_link = split->connected ? ZMK_DUAL_DISPLAY_SPLIT_LINK_CONNECTED
                                             : ZMK_DUAL_DISPLAY_SPLIT_LINK_DISCONNECTED;
        ZMK_DUAL_DISPLAY_LOG_DBG("applied split peripheral event to display state: connected=%d",
                                 split->connected);
        return true;
    }
#endif

    ZMK_DUAL_DISPLAY_LOG_DBG("ignored unhandled display state event: %s", eh->event->name);
    return false;
}

static void firmware_render_work_cb(struct k_work *work) {
    ARG_UNUSED(work);

    struct zmk_dual_display_state state;
    k_mutex_lock(&firmware_state_mutex, K_FOREVER);
    state = firmware_state;
    k_mutex_unlock(&firmware_state_mutex);

    const int err = zmk_dual_display_status_screen_update_from_state(&state);
    if (err < 0) {
        ZMK_DUAL_DISPLAY_LOG_WRN("display state refresh did not render: err=%d", err);
    }
}

static int firmware_state_listener_cb(const zmk_event_t *eh) {
    struct zmk_dual_display_state previous;
    struct zmk_dual_display_state next;

    k_mutex_lock(&firmware_state_mutex, K_FOREVER);
    if (!firmware_state_ready) {
        k_mutex_unlock(&firmware_state_mutex);
        ZMK_DUAL_DISPLAY_LOG_DBG("ignored display event before status screen init: %s",
                                 eh == NULL ? "unknown" : eh->event->name);
        return ZMK_EV_EVENT_BUBBLE;
    }

    previous = firmware_state;
    next = firmware_state;
    const bool handled = apply_event_to_state(eh, &next);
    const bool changed = handled && !states_equal(&previous, &next);
    if (changed) {
        firmware_state = next;
    }
    k_mutex_unlock(&firmware_state_mutex);

    if (!handled) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (!changed) {
        ZMK_DUAL_DISPLAY_LOG_DBG("display state event produced no visual state change: %s",
                                 eh->event->name);
        return ZMK_EV_EVENT_BUBBLE;
    }

    zmk_dual_display_log_state_transition(&previous, &next);

    if (!zmk_display_is_initialized()) {
        ZMK_DUAL_DISPLAY_LOG_DBG("display event updated state before display init: %s",
                                 eh->event->name);
        return ZMK_EV_EVENT_BUBBLE;
    }

    k_work_submit_to_queue(zmk_display_work_q(), &firmware_render_work);
    ZMK_DUAL_DISPLAY_LOG_DBG("queued display refresh for event: %s", eh->event->name);

    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(dual_display_firmware_state, firmware_state_listener_cb);
ZMK_SUBSCRIPTION(dual_display_firmware_state, zmk_activity_state_changed);

#if IS_ENABLED(CONFIG_ZMK_BATTERY_REPORTING)
ZMK_SUBSCRIPTION(dual_display_firmware_state, zmk_battery_state_changed);
#endif

#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(dual_display_firmware_state, zmk_usb_conn_state_changed);
#endif

#if !IS_ENABLED(CONFIG_ZMK_SPLIT) || IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
ZMK_SUBSCRIPTION(dual_display_firmware_state, zmk_endpoint_changed);
ZMK_SUBSCRIPTION(dual_display_firmware_state, zmk_keycode_state_changed);
ZMK_SUBSCRIPTION(dual_display_firmware_state, zmk_layer_state_changed);

#if IS_ENABLED(CONFIG_ZMK_BLE)
ZMK_SUBSCRIPTION(dual_display_firmware_state, zmk_ble_active_profile_changed);
#endif
#endif

#if IS_ENABLED(CONFIG_ZMK_SPLIT)
ZMK_SUBSCRIPTION(dual_display_firmware_state, zmk_split_peripheral_status_changed);
#endif
