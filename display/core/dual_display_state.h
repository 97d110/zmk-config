/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

enum zmk_dual_display_side {
    ZMK_DUAL_DISPLAY_SIDE_LEFT,
    ZMK_DUAL_DISPLAY_SIDE_RIGHT,
};

enum zmk_dual_display_role {
    ZMK_DUAL_DISPLAY_ROLE_UNKNOWN,
    ZMK_DUAL_DISPLAY_ROLE_CENTRAL,
    ZMK_DUAL_DISPLAY_ROLE_PERIPHERAL,
};

enum zmk_dual_display_battery_bucket {
    ZMK_DUAL_DISPLAY_BATTERY_UNKNOWN,
    ZMK_DUAL_DISPLAY_BATTERY_0_10,
    ZMK_DUAL_DISPLAY_BATTERY_11_50,
    ZMK_DUAL_DISPLAY_BATTERY_51_100,
    ZMK_DUAL_DISPLAY_BATTERY_0_10_CHARGING,
    ZMK_DUAL_DISPLAY_BATTERY_11_50_CHARGING,
    ZMK_DUAL_DISPLAY_BATTERY_51_100_CHARGING,
};

enum zmk_dual_display_activity_bucket {
    ZMK_DUAL_DISPLAY_ACTIVITY_IDLE,
    ZMK_DUAL_DISPLAY_ACTIVITY_SLEEP,
    ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_2S,
    ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_5S,
    ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_10S,
    ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_15S,
};

enum zmk_dual_display_transport_state {
    ZMK_DUAL_DISPLAY_TRANSPORT_UNKNOWN,
    ZMK_DUAL_DISPLAY_TRANSPORT_USB,
    ZMK_DUAL_DISPLAY_TRANSPORT_BT,
    ZMK_DUAL_DISPLAY_TRANSPORT_DISCONNECTED,
};

enum zmk_dual_display_split_link_state {
    ZMK_DUAL_DISPLAY_SPLIT_LINK_UNKNOWN,
    ZMK_DUAL_DISPLAY_SPLIT_LINK_CONNECTED,
    ZMK_DUAL_DISPLAY_SPLIT_LINK_DISCONNECTED,
};

enum zmk_dual_display_layer_mode {
    ZMK_DUAL_DISPLAY_LAYER_UNKNOWN,
    ZMK_DUAL_DISPLAY_LAYER_TYPE,
    ZMK_DUAL_DISPLAY_LAYER_SYMBOL,
    ZMK_DUAL_DISPLAY_LAYER_MOD,
    ZMK_DUAL_DISPLAY_LAYER_CONFIG,
};

struct zmk_dual_display_state {
    enum zmk_dual_display_side side;
    enum zmk_dual_display_role role;
    enum zmk_dual_display_battery_bucket battery;
    enum zmk_dual_display_activity_bucket activity;
    enum zmk_dual_display_transport_state transport;
    enum zmk_dual_display_split_link_state split_link;
    enum zmk_dual_display_layer_mode layer;
};

const char *zmk_dual_display_side_name(enum zmk_dual_display_side side);

enum zmk_dual_display_side zmk_dual_display_normalize_side(enum zmk_dual_display_side side);

void zmk_dual_display_default_state(enum zmk_dual_display_side side,
                                    struct zmk_dual_display_state *out_state);

enum zmk_dual_display_layer_mode zmk_dual_display_layer_mode_from_index(uint8_t layer);

enum zmk_dual_display_activity_bucket
zmk_dual_display_activity_bucket_from_typing_streak(uint32_t typing_streak_ms, bool sleeping);

enum zmk_dual_display_battery_bucket zmk_dual_display_battery_bucket_from_percent(
    int16_t percent, bool charging);

enum zmk_dual_display_transport_state zmk_dual_display_transport_state_from_flags(bool usb,
                                                                                  bool bt);

void zmk_dual_display_log_state_transition(const struct zmk_dual_display_state *previous,
                                           const struct zmk_dual_display_state *current);
