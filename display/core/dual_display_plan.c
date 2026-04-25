/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>

#include <display/core/dual_display_plan.h>
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

static struct zmk_dual_display_rect rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    return (struct zmk_dual_display_rect){
        .x = x,
        .y = y,
        .width = width,
        .height = height,
    };
}

static void build_status_bar_plan(enum zmk_dual_display_side side,
                                  struct zmk_dual_display_status_bar_plan *out_plan) {
    out_plan->bounds = rect(0, 0, ZMK_DUAL_DISPLAY_WIDTH, ZMK_DUAL_DISPLAY_STATUS_BAR_HEIGHT);
    out_plan->slot_count = ZMK_DUAL_DISPLAY_STATUS_SLOT_COUNT;

    out_plan->slots[0] = (struct zmk_dual_display_status_slot_plan){
        .kind = ZMK_DUAL_DISPLAY_STATUS_SLOT_BATTERY,
        .bounds = rect(6, 3, 18, 8),
        .active = true,
    };

    out_plan->slots[1] = (struct zmk_dual_display_status_slot_plan){
        .kind = ZMK_DUAL_DISPLAY_STATUS_SLOT_SPLIT_LINK,
        .bounds = rect(74, 3, 12, 8),
        .active = true,
    };

    out_plan->slots[2] = (struct zmk_dual_display_status_slot_plan){
        .kind = side == ZMK_DUAL_DISPLAY_SIDE_LEFT ? ZMK_DUAL_DISPLAY_STATUS_SLOT_TRANSPORT
                                                   : ZMK_DUAL_DISPLAY_STATUS_SLOT_LAYER_MODE,
        .bounds = rect(140, 3, 12, 8),
        .active = true,
    };

    ZMK_DUAL_DISPLAY_LOG_DBG("planned %s status bar: %u slots",
                             zmk_dual_display_side_name(side),
                             (unsigned int)out_plan->slot_count);
}

static void build_animation_plan(enum zmk_dual_display_side side,
                                 struct zmk_dual_display_animation_plan *out_plan) {
    out_plan->bounds = rect(0, ZMK_DUAL_DISPLAY_ANIMATION_Y, ZMK_DUAL_DISPLAY_WIDTH,
                            ZMK_DUAL_DISPLAY_ANIMATION_HEIGHT);
    out_plan->use_alternate_side_variant = side == ZMK_DUAL_DISPLAY_SIDE_RIGHT;
    out_plan->asset = out_plan->use_alternate_side_variant
                          ? ZMK_DUAL_DISPLAY_PLACEHOLDER_ASSET_STACKED_SCENE_ALT
                          : ZMK_DUAL_DISPLAY_PLACEHOLDER_ASSET_STACKED_SCENE;

    ZMK_DUAL_DISPLAY_LOG_DBG("planned %s animation: asset=%d alternate_variant=%d "
                             "bounds=%ux%u+%u+%u",
                             zmk_dual_display_side_name(side), out_plan->asset,
                             out_plan->use_alternate_side_variant,
                             (unsigned int)out_plan->bounds.width,
                             (unsigned int)out_plan->bounds.height,
                             (unsigned int)out_plan->bounds.x, (unsigned int)out_plan->bounds.y);
}

void zmk_dual_display_build_screen_plan(enum zmk_dual_display_side side,
                                        struct zmk_dual_display_screen_plan *out_plan) {
    if (out_plan == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("screen plan output was NULL");
        return;
    }

    if (side != ZMK_DUAL_DISPLAY_SIDE_LEFT && side != ZMK_DUAL_DISPLAY_SIDE_RIGHT) {
        ZMK_DUAL_DISPLAY_LOG_WRN("falling back to left plan for invalid side: %d", side);
        side = ZMK_DUAL_DISPLAY_SIDE_LEFT;
    }

    out_plan->side = side;
    build_status_bar_plan(side, &out_plan->status_bar);
    build_animation_plan(side, &out_plan->animation);

    ZMK_DUAL_DISPLAY_LOG_DBG("built %s screen plan", zmk_dual_display_side_name(side));
}

void zmk_dual_display_build_dual_plan(struct zmk_dual_display_dual_plan *out_plan) {
    if (out_plan == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("dual plan output was NULL");
        return;
    }

    zmk_dual_display_build_screen_plan(ZMK_DUAL_DISPLAY_SIDE_LEFT, &out_plan->left);
    zmk_dual_display_build_screen_plan(ZMK_DUAL_DISPLAY_SIDE_RIGHT, &out_plan->right);

    ZMK_DUAL_DISPLAY_LOG_DBG("built dual screen plan");
}
