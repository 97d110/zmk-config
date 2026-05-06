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
static bool typing_period_active;
static bool typing_period_had_keypress;
static uint32_t typing_activity_seconds;

static void firmware_render_work_cb(struct k_work *work);
static void typing_activity_work_cb(struct k_work *work);

K_MUTEX_DEFINE(firmware_state_mutex);
K_WORK_DEFINE(firmware_render_work, firmware_render_work_cb);
K_WORK_DELAYABLE_DEFINE(typing_activity_work, typing_activity_work_cb);

#define TYPING_ACTIVITY_PERIOD K_SECONDS(1)

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

static bool record_typing_keypress_locked(void) {
    typing_period_had_keypress = true;

    if (!typing_period_active) {
        typing_period_active = true;
        typing_activity_seconds = 0;
        return true;
    }

    return false;
}
#endif

static void reset_typing_activity_locked(void) {
    typing_period_active = false;
    typing_period_had_keypress = false;
    typing_activity_seconds = 0;
}

static const char *role_name(enum zmk_dual_display_role role) {
    switch (role) {
    case ZMK_DUAL_DISPLAY_ROLE_CENTRAL:
        return "central";
    case ZMK_DUAL_DISPLAY_ROLE_PERIPHERAL:
        return "peripheral";
    case ZMK_DUAL_DISPLAY_ROLE_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *battery_name(enum zmk_dual_display_battery_bucket battery) {
    switch (battery) {
    case ZMK_DUAL_DISPLAY_BATTERY_0_10:
        return "0_10";
    case ZMK_DUAL_DISPLAY_BATTERY_11_50:
        return "11_50";
    case ZMK_DUAL_DISPLAY_BATTERY_51_100:
        return "51_100";
    case ZMK_DUAL_DISPLAY_BATTERY_0_10_CHARGING:
        return "0_10_charging";
    case ZMK_DUAL_DISPLAY_BATTERY_11_50_CHARGING:
        return "11_50_charging";
    case ZMK_DUAL_DISPLAY_BATTERY_51_100_CHARGING:
        return "51_100_charging";
    case ZMK_DUAL_DISPLAY_BATTERY_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *activity_name(enum zmk_dual_display_activity_bucket activity) {
    switch (activity) {
    case ZMK_DUAL_DISPLAY_ACTIVITY_IDLE:
        return "idle";
    case ZMK_DUAL_DISPLAY_ACTIVITY_SLEEP:
        return "sleep";
    case ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_2S:
        return "typing_2s";
    case ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_5S:
        return "typing_5s";
    case ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_10S:
        return "typing_10s";
    case ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_15S:
        return "typing_15s";
    default:
        return "unknown";
    }
}

static const char *transport_name(enum zmk_dual_display_transport_state transport) {
    switch (transport) {
    case ZMK_DUAL_DISPLAY_TRANSPORT_USB:
        return "usb";
    case ZMK_DUAL_DISPLAY_TRANSPORT_BT:
        return "bt";
    case ZMK_DUAL_DISPLAY_TRANSPORT_DISCONNECTED:
        return "disconnected";
    case ZMK_DUAL_DISPLAY_TRANSPORT_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *split_link_name(enum zmk_dual_display_split_link_state split_link) {
    switch (split_link) {
    case ZMK_DUAL_DISPLAY_SPLIT_LINK_CONNECTED:
        return "connected";
    case ZMK_DUAL_DISPLAY_SPLIT_LINK_DISCONNECTED:
        return "disconnected";
    case ZMK_DUAL_DISPLAY_SPLIT_LINK_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *layer_name(enum zmk_dual_display_layer_mode layer) {
    switch (layer) {
    case ZMK_DUAL_DISPLAY_LAYER_TYPE:
        return "type";
    case ZMK_DUAL_DISPLAY_LAYER_SYMBOL:
        return "symbol";
    case ZMK_DUAL_DISPLAY_LAYER_MOD:
        return "mod";
    case ZMK_DUAL_DISPLAY_LAYER_CONFIG:
        return "config";
    case ZMK_DUAL_DISPLAY_LAYER_UNKNOWN:
    default:
        return "unknown";
    }
}

static void log_complete_state(const char *reason, const struct zmk_dual_display_state *state,
                               uint32_t typing_seconds, bool period_had_keypress) {
    if (state == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("cannot log complete display state without state");
        return;
    }

    ZMK_DUAL_DISPLAY_LOG_DBG(
        "complete display state: reason=%s side=%s role=%s battery=%s activity=%s transport=%s split=%s layer=%s typing_seconds=%u period_keypress=%d",
        reason, zmk_dual_display_side_name(state->side), role_name(state->role),
        battery_name(state->battery), activity_name(state->activity),
        transport_name(state->transport), split_link_name(state->split_link),
        layer_name(state->layer), (unsigned int)typing_seconds, period_had_keypress);
}

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

static bool apply_event_to_state(const zmk_event_t *eh, struct zmk_dual_display_state *state,
                                 bool *start_typing_cycle, bool *cancel_typing_cycle,
                                 bool *quiet_no_change_log) {
    if (eh == NULL || state == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("ignored display state event update with missing input");
        return false;
    }

    const struct zmk_activity_state_changed *activity = as_zmk_activity_state_changed(eh);
    if (activity != NULL) {
        if (activity->state == ZMK_ACTIVITY_ACTIVE) {
            ZMK_DUAL_DISPLAY_LOG_DBG(
                "applied active event without changing typing cycle state");
            return true;
        }

        reset_typing_activity_locked();
        if (cancel_typing_cycle != NULL) {
            *cancel_typing_cycle = true;
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
        if (quiet_no_change_log != NULL) {
            *quiet_no_change_log = true;
        }

        if (!keycode->state) {
            return true;
        }

        if (start_typing_cycle != NULL && record_typing_keypress_locked()) {
            *start_typing_cycle = true;
        }
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

static void typing_activity_work_cb(struct k_work *work) {
    ARG_UNUSED(work);

    struct zmk_dual_display_state previous;
    struct zmk_dual_display_state next;
    bool period_had_keypress;
    bool changed = false;
    bool reschedule = false;
    uint32_t typing_seconds;

    k_mutex_lock(&firmware_state_mutex, K_FOREVER);

    previous = firmware_state;
    next = firmware_state;
    period_had_keypress = typing_period_had_keypress;

    if (typing_period_active && period_had_keypress) {
        typing_period_had_keypress = false;
        typing_activity_seconds++;
        typing_seconds = typing_activity_seconds;
        next.activity = zmk_dual_display_activity_bucket_from_typing_streak(
            typing_activity_seconds * 1000U, false);
        changed = !states_equal(&previous, &next);
        if (changed) {
            firmware_state = next;
        }
        reschedule = true;
        log_complete_state("typing-period-complete", &next, typing_seconds, period_had_keypress);
    } else {
        typing_seconds = typing_activity_seconds;
        reset_typing_activity_locked();
        log_complete_state("typing-decay-pending", &next, typing_seconds, period_had_keypress);
    }

    k_mutex_unlock(&firmware_state_mutex);

    if (changed) {
        zmk_dual_display_log_state_transition(&previous, &next);
        const int err = zmk_dual_display_status_screen_update_from_state(&next);
        if (err < 0) {
            ZMK_DUAL_DISPLAY_LOG_WRN("typing activity refresh did not render: err=%d", err);
        }
    }

    if (reschedule) {
        const int err =
            k_work_schedule_for_queue(zmk_display_work_q(), &typing_activity_work,
                                      TYPING_ACTIVITY_PERIOD);
        if (err < 0) {
            ZMK_DUAL_DISPLAY_LOG_WRN("failed to schedule typing activity period: err=%d", err);
        }
    }
}

static int firmware_state_listener_cb(const zmk_event_t *eh) {
    struct zmk_dual_display_state previous;
    struct zmk_dual_display_state next;
    bool start_typing_cycle = false;
    bool cancel_typing_cycle = false;
    bool quiet_no_change_log = false;

    k_mutex_lock(&firmware_state_mutex, K_FOREVER);
    if (!firmware_state_ready) {
        k_mutex_unlock(&firmware_state_mutex);
        ZMK_DUAL_DISPLAY_LOG_DBG("ignored display event before status screen init: %s",
                                 eh == NULL ? "unknown" : eh->event->name);
        return ZMK_EV_EVENT_BUBBLE;
    }

    previous = firmware_state;
    next = firmware_state;
    const bool handled = apply_event_to_state(eh, &next, &start_typing_cycle, &cancel_typing_cycle,
                                              &quiet_no_change_log);
    const bool changed = handled && !states_equal(&previous, &next);
    if (changed) {
        firmware_state = next;
    }
    k_mutex_unlock(&firmware_state_mutex);

    if (!handled) {
        return ZMK_EV_EVENT_BUBBLE;
    }

    if (cancel_typing_cycle) {
        k_work_cancel_delayable(&typing_activity_work);
    }

    if (start_typing_cycle) {
        if (!zmk_display_is_initialized()) {
            k_mutex_lock(&firmware_state_mutex, K_FOREVER);
            reset_typing_activity_locked();
            k_mutex_unlock(&firmware_state_mutex);
            ZMK_DUAL_DISPLAY_LOG_DBG("ignored typing activity cycle before display init");
        } else {
            const int err =
                k_work_schedule_for_queue(zmk_display_work_q(), &typing_activity_work,
                                          TYPING_ACTIVITY_PERIOD);
            if (err < 0) {
                ZMK_DUAL_DISPLAY_LOG_WRN("failed to start typing activity cycle: err=%d", err);
            }
        }
    }

    if (!changed) {
        if (!quiet_no_change_log) {
            ZMK_DUAL_DISPLAY_LOG_DBG("display state event produced no visual state change: %s",
                                     eh->event->name);
        }
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
