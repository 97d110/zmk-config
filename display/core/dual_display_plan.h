/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

#include <display/core/dual_display_state.h>

#define ZMK_DUAL_DISPLAY_SHORT_EDGE 68
#define ZMK_DUAL_DISPLAY_LONG_EDGE 160
#define ZMK_DUAL_DISPLAY_WIDTH ZMK_DUAL_DISPLAY_SHORT_EDGE
#define ZMK_DUAL_DISPLAY_HEIGHT ZMK_DUAL_DISPLAY_LONG_EDGE
#define ZMK_DUAL_DISPLAY_STATUS_BAR_HEIGHT 14
#define ZMK_DUAL_DISPLAY_ANIMATION_Y ZMK_DUAL_DISPLAY_STATUS_BAR_HEIGHT
#define ZMK_DUAL_DISPLAY_ANIMATION_HEIGHT \
    (ZMK_DUAL_DISPLAY_HEIGHT - ZMK_DUAL_DISPLAY_STATUS_BAR_HEIGHT)
#define ZMK_DUAL_DISPLAY_STATUS_SLOT_COUNT 3

enum zmk_dual_display_status_slot_kind {
    ZMK_DUAL_DISPLAY_STATUS_SLOT_BATTERY,
    ZMK_DUAL_DISPLAY_STATUS_SLOT_SPLIT_LINK,
    ZMK_DUAL_DISPLAY_STATUS_SLOT_TRANSPORT,
    ZMK_DUAL_DISPLAY_STATUS_SLOT_LAYER_MODE,
};

enum zmk_dual_display_status_slot_value {
    ZMK_DUAL_DISPLAY_STATUS_VALUE_UNKNOWN,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_0_10,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_11_50,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_51_100,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_0_10_CHARGING,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_11_50_CHARGING,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_51_100_CHARGING,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_SPLIT_CONNECTED,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_SPLIT_DISCONNECTED,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_TRANSPORT_USB,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_TRANSPORT_BT,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_TRANSPORT_DISCONNECTED,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_TYPE,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_SYMBOL,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_MOD,
    ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_CONFIG,
};

enum zmk_dual_display_scene_variant {
    ZMK_DUAL_DISPLAY_SCENE_VARIANT_PRIMARY,
    ZMK_DUAL_DISPLAY_SCENE_VARIANT_SECONDARY,
};

struct zmk_dual_display_rect {
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
};

struct zmk_dual_display_status_slot_plan {
    enum zmk_dual_display_status_slot_kind kind;
    enum zmk_dual_display_status_slot_value value;
    struct zmk_dual_display_rect bounds;
};

struct zmk_dual_display_status_bar_plan {
    struct zmk_dual_display_rect bounds;
    uint8_t slot_count;
    struct zmk_dual_display_status_slot_plan slots[ZMK_DUAL_DISPLAY_STATUS_SLOT_COUNT];
};

struct zmk_dual_display_animation_plan {
    struct zmk_dual_display_rect bounds;
    enum zmk_dual_display_scene_variant variant;
    enum zmk_dual_display_activity_bucket activity;
};

struct zmk_dual_display_screen_plan {
    enum zmk_dual_display_side side;
    struct zmk_dual_display_status_bar_plan status_bar;
    struct zmk_dual_display_animation_plan animation;
};

struct zmk_dual_display_dual_plan {
    struct zmk_dual_display_screen_plan left;
    struct zmk_dual_display_screen_plan right;
};

void zmk_dual_display_build_screen_plan_from_state(
    const struct zmk_dual_display_state *state, struct zmk_dual_display_screen_plan *out_plan);

void zmk_dual_display_build_screen_plan(enum zmk_dual_display_side side,
                                        struct zmk_dual_display_screen_plan *out_plan);

void zmk_dual_display_build_dual_plan_from_state(
    const struct zmk_dual_display_state *left_state,
    const struct zmk_dual_display_state *right_state, struct zmk_dual_display_dual_plan *out_plan);

void zmk_dual_display_build_dual_plan(struct zmk_dual_display_dual_plan *out_plan);
