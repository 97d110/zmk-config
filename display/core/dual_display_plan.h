/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <display/assets/placeholder_assets.h>

#define ZMK_DUAL_DISPLAY_WIDTH 160
#define ZMK_DUAL_DISPLAY_HEIGHT 68
#define ZMK_DUAL_DISPLAY_STATUS_BAR_HEIGHT 14
#define ZMK_DUAL_DISPLAY_ANIMATION_Y ZMK_DUAL_DISPLAY_STATUS_BAR_HEIGHT
#define ZMK_DUAL_DISPLAY_ANIMATION_HEIGHT \
    (ZMK_DUAL_DISPLAY_HEIGHT - ZMK_DUAL_DISPLAY_STATUS_BAR_HEIGHT)
#define ZMK_DUAL_DISPLAY_STATUS_SLOT_COUNT 3

enum zmk_dual_display_side {
    ZMK_DUAL_DISPLAY_SIDE_LEFT,
    ZMK_DUAL_DISPLAY_SIDE_RIGHT,
};

enum zmk_dual_display_status_slot_kind {
    ZMK_DUAL_DISPLAY_STATUS_SLOT_BATTERY,
    ZMK_DUAL_DISPLAY_STATUS_SLOT_SPLIT_LINK,
    ZMK_DUAL_DISPLAY_STATUS_SLOT_TRANSPORT,
    ZMK_DUAL_DISPLAY_STATUS_SLOT_LAYER_MODE,
};

struct zmk_dual_display_rect {
    uint8_t x;
    uint8_t y;
    uint8_t width;
    uint8_t height;
};

struct zmk_dual_display_status_slot_plan {
    enum zmk_dual_display_status_slot_kind kind;
    struct zmk_dual_display_rect bounds;
    bool active;
};

struct zmk_dual_display_status_bar_plan {
    struct zmk_dual_display_rect bounds;
    uint8_t slot_count;
    struct zmk_dual_display_status_slot_plan slots[ZMK_DUAL_DISPLAY_STATUS_SLOT_COUNT];
};

struct zmk_dual_display_animation_plan {
    struct zmk_dual_display_rect bounds;
    enum zmk_dual_display_placeholder_asset asset;
    bool mirror;
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

const char *zmk_dual_display_side_name(enum zmk_dual_display_side side);

void zmk_dual_display_build_screen_plan(enum zmk_dual_display_side side,
                                        struct zmk_dual_display_screen_plan *out_plan);

void zmk_dual_display_build_dual_plan(struct zmk_dual_display_dual_plan *out_plan);
