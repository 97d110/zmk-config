/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>

#include <display/core/dual_display_state.h>
#include <display/log.h>

const char *zmk_dual_display_side_name(enum zmk_dual_display_side side) {
    switch (side) {
    case ZMK_DUAL_DISPLAY_SIDE_LEFT:
        return "left";
    case ZMK_DUAL_DISPLAY_SIDE_RIGHT:
        return "right";
    default:
        ZMK_DUAL_DISPLAY_LOG_WRN("unknown display side: %d", side);
        return "unknown";
    }
}

enum zmk_dual_display_side zmk_dual_display_normalize_side(enum zmk_dual_display_side side) {
    switch (side) {
    case ZMK_DUAL_DISPLAY_SIDE_LEFT:
    case ZMK_DUAL_DISPLAY_SIDE_RIGHT:
        return side;
    default:
        ZMK_DUAL_DISPLAY_LOG_WRN("falling back to left display side for invalid side: %d", side);
        return ZMK_DUAL_DISPLAY_SIDE_LEFT;
    }
}

void zmk_dual_display_default_state(enum zmk_dual_display_side side,
                                    struct zmk_dual_display_state *out_state) {
    if (out_state == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("default state output was NULL");
        return;
    }

    const enum zmk_dual_display_side normalized_side = zmk_dual_display_normalize_side(side);

    *out_state = (struct zmk_dual_display_state){
        .side = normalized_side,
        .role = normalized_side == ZMK_DUAL_DISPLAY_SIDE_LEFT
                    ? ZMK_DUAL_DISPLAY_ROLE_CENTRAL
                    : ZMK_DUAL_DISPLAY_ROLE_PERIPHERAL,
        .battery = ZMK_DUAL_DISPLAY_BATTERY_UNKNOWN,
        .activity = ZMK_DUAL_DISPLAY_ACTIVITY_IDLE,
        .transport = ZMK_DUAL_DISPLAY_TRANSPORT_UNKNOWN,
        .split_link = ZMK_DUAL_DISPLAY_SPLIT_LINK_UNKNOWN,
        .layer = ZMK_DUAL_DISPLAY_LAYER_TYPE,
    };

    ZMK_DUAL_DISPLAY_LOG_DBG("built default %s display state",
                             zmk_dual_display_side_name(normalized_side));
}

enum zmk_dual_display_layer_mode zmk_dual_display_layer_mode_from_index(uint8_t layer) {
    switch (layer) {
    case 0:
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped layer index 0 to type mode");
        return ZMK_DUAL_DISPLAY_LAYER_TYPE;
    case 1:
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped layer index 1 to symbol mode");
        return ZMK_DUAL_DISPLAY_LAYER_SYMBOL;
    case 2:
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped layer index 2 to mod mode");
        return ZMK_DUAL_DISPLAY_LAYER_MOD;
    case 3:
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped layer index 3 to config mode");
        return ZMK_DUAL_DISPLAY_LAYER_CONFIG;
    default:
        ZMK_DUAL_DISPLAY_LOG_WRN("mapped invalid layer index %u to unknown mode",
                                 (unsigned int)layer);
        return ZMK_DUAL_DISPLAY_LAYER_UNKNOWN;
    }
}

enum zmk_dual_display_activity_bucket
zmk_dual_display_activity_bucket_from_typing_streak(uint32_t typing_streak_ms, bool sleeping) {
    if (sleeping) {
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped sleeping state to sleep activity bucket");
        return ZMK_DUAL_DISPLAY_ACTIVITY_SLEEP;
    }

    if (typing_streak_ms == 0) {
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped empty typing streak to idle activity bucket");
        return ZMK_DUAL_DISPLAY_ACTIVITY_IDLE;
    }

    if (typing_streak_ms <= 2000) {
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped typing streak %u ms to 2s activity bucket",
                                 (unsigned int)typing_streak_ms);
        return ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_2S;
    }

    if (typing_streak_ms <= 5000) {
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped typing streak %u ms to 5s activity bucket",
                                 (unsigned int)typing_streak_ms);
        return ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_5S;
    }

    if (typing_streak_ms <= 10000) {
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped typing streak %u ms to 10s activity bucket",
                                 (unsigned int)typing_streak_ms);
        return ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_10S;
    }

    ZMK_DUAL_DISPLAY_LOG_DBG("mapped typing streak %u ms to capped 15s activity bucket",
                             (unsigned int)typing_streak_ms);
    return ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_15S;
}

enum zmk_dual_display_battery_bucket zmk_dual_display_battery_bucket_from_percent(
    int16_t percent, bool charging) {
    if (percent < 0 || percent > 100) {
        ZMK_DUAL_DISPLAY_LOG_WRN("mapped invalid battery percent %d to unknown bucket", percent);
        return ZMK_DUAL_DISPLAY_BATTERY_UNKNOWN;
    }

    enum zmk_dual_display_battery_bucket bucket;

    if (percent <= 10) {
        bucket = charging ? ZMK_DUAL_DISPLAY_BATTERY_0_10_CHARGING
                          : ZMK_DUAL_DISPLAY_BATTERY_0_10;
    } else if (percent <= 50) {
        bucket = charging ? ZMK_DUAL_DISPLAY_BATTERY_11_50_CHARGING
                          : ZMK_DUAL_DISPLAY_BATTERY_11_50;
    } else {
        bucket = charging ? ZMK_DUAL_DISPLAY_BATTERY_51_100_CHARGING
                          : ZMK_DUAL_DISPLAY_BATTERY_51_100;
    }

    ZMK_DUAL_DISPLAY_LOG_DBG("mapped battery percent %d charging=%d to bucket=%d", percent,
                             charging, bucket);
    return bucket;
}

enum zmk_dual_display_transport_state zmk_dual_display_transport_state_from_flags(bool usb,
                                                                                  bool bt) {
    if (usb) {
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped transport flags usb=%d bt=%d to USB", usb, bt);
        return ZMK_DUAL_DISPLAY_TRANSPORT_USB;
    }

    if (bt) {
        ZMK_DUAL_DISPLAY_LOG_DBG("mapped transport flags usb=%d bt=%d to BT", usb, bt);
        return ZMK_DUAL_DISPLAY_TRANSPORT_BT;
    }

    ZMK_DUAL_DISPLAY_LOG_DBG("mapped transport flags usb=%d bt=%d to disconnected", usb, bt);
    return ZMK_DUAL_DISPLAY_TRANSPORT_DISCONNECTED;
}

void zmk_dual_display_log_state_transition(const struct zmk_dual_display_state *previous,
                                           const struct zmk_dual_display_state *current) {
    if (current == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("cannot log display state transition without current state");
        return;
    }

    if (previous == NULL) {
        ZMK_DUAL_DISPLAY_LOG_DBG(
            "display state initialized: side=%d role=%d battery=%d activity=%d transport=%d split=%d layer=%d",
            current->side, current->role, current->battery, current->activity, current->transport,
            current->split_link, current->layer);
        return;
    }

    if (previous->side != current->side) {
        ZMK_DUAL_DISPLAY_LOG_DBG("display state side changed: %d -> %d", previous->side,
                                 current->side);
    }
    if (previous->role != current->role) {
        ZMK_DUAL_DISPLAY_LOG_DBG("display state role changed: %d -> %d", previous->role,
                                 current->role);
    }
    if (previous->battery != current->battery) {
        ZMK_DUAL_DISPLAY_LOG_DBG("display state battery changed: %d -> %d", previous->battery,
                                 current->battery);
    }
    if (previous->activity != current->activity) {
        ZMK_DUAL_DISPLAY_LOG_DBG("display state activity changed: %d -> %d", previous->activity,
                                 current->activity);
    }
    if (previous->transport != current->transport) {
        ZMK_DUAL_DISPLAY_LOG_DBG("display state transport changed: %d -> %d",
                                 previous->transport, current->transport);
    }
    if (previous->split_link != current->split_link) {
        ZMK_DUAL_DISPLAY_LOG_DBG("display state split link changed: %d -> %d",
                                 previous->split_link, current->split_link);
    }
    if (previous->layer != current->layer) {
        ZMK_DUAL_DISPLAY_LOG_DBG("display state layer changed: %d -> %d", previous->layer,
                                 current->layer);
    }
}
