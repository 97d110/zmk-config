/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <display/mock/lvgl/placeholder_renderer.h>

#include <display/log.h>

static void clear_obj_defaults(lv_obj_t *obj) {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
}

static lv_obj_t *add_rect(lv_obj_t *parent, const struct zmk_dual_display_rect *bounds,
                          bool filled) {
    lv_obj_t *obj = lv_obj_create(parent);

    clear_obj_defaults(obj);
    lv_obj_set_pos(obj, bounds->x, bounds->y);
    lv_obj_set_size(obj, bounds->width, bounds->height);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(obj, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, filled ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);

    return obj;
}

static void render_status_slot(lv_obj_t *screen,
                               const struct zmk_dual_display_status_slot_plan *slot) {
    add_rect(screen, &slot->bounds, false);

    if (slot->active) {
        struct zmk_dual_display_rect active_mark = {
            .x = slot->bounds.x + slot->bounds.width - 4,
            .y = slot->bounds.y + 2,
            .width = 2,
            .height = slot->bounds.height - 4,
        };
        add_rect(screen, &active_mark, true);
    }

    ZMK_DUAL_DISPLAY_LOG_DBG("mock rendered status slot kind=%d active=%d at %u,%u %ux%u",
                             slot->kind, slot->active, (unsigned int)slot->bounds.x,
                             (unsigned int)slot->bounds.y, (unsigned int)slot->bounds.width,
                             (unsigned int)slot->bounds.height);
}

static void render_status_bar(lv_obj_t *screen,
                              const struct zmk_dual_display_status_bar_plan *plan) {
    add_rect(screen, &plan->bounds, false);

    for (uint8_t i = 0; i < plan->slot_count && i < ZMK_DUAL_DISPLAY_STATUS_SLOT_COUNT; i++) {
        render_status_slot(screen, &plan->slots[i]);
    }

    const struct zmk_dual_display_rect divider = {
        .x = 0,
        .y = plan->bounds.y + plan->bounds.height - 1,
        .width = ZMK_DUAL_DISPLAY_WIDTH,
        .height = 1,
    };
    add_rect(screen, &divider, true);

    ZMK_DUAL_DISPLAY_LOG_DBG("mock rendered status bar at %u,%u %ux%u with %u slots",
                             (unsigned int)plan->bounds.x, (unsigned int)plan->bounds.y,
                             (unsigned int)plan->bounds.width,
                             (unsigned int)plan->bounds.height,
                             (unsigned int)plan->slot_count);
}

static void render_animation_region(lv_obj_t *screen,
                                    const struct zmk_dual_display_animation_plan *plan) {
    add_rect(screen, &plan->bounds, false);

    const bool secondary_variant = plan->variant == ZMK_DUAL_DISPLAY_SCENE_VARIANT_SECONDARY;
    const uint8_t cue_x = secondary_variant ? 136 : 18;

    struct zmk_dual_display_rect upper_motion_band = {
        .x = 42,
        .y = plan->bounds.y + 8,
        .width = 76,
        .height = 8,
    };
    struct zmk_dual_display_rect center_frame = {
        .x = 58,
        .y = plan->bounds.y + 22,
        .width = 44,
        .height = 20,
    };
    struct zmk_dual_display_rect side_cue = {
        .x = cue_x,
        .y = plan->bounds.y + 28,
        .width = 6,
        .height = 6,
    };
    struct zmk_dual_display_rect lower_motion_band = {
        .x = 26,
        .y = plan->bounds.y + plan->bounds.height - 9,
        .width = 108,
        .height = 3,
    };

    add_rect(screen, &upper_motion_band, true);
    add_rect(screen, &center_frame, false);
    add_rect(screen, &side_cue, true);
    add_rect(screen, &lower_motion_band, true);

    ZMK_DUAL_DISPLAY_LOG_DBG("mock rendered top-down animation placeholder variant=%d",
                             plan->variant);
}

void zmk_dual_display_mock_lvgl_render_screen_plan(
    lv_obj_t *screen, const struct zmk_dual_display_screen_plan *plan) {
    ZMK_DUAL_DISPLAY_LOG_DBG("mock rendering %s placeholder screen plan",
                             zmk_dual_display_side_name(plan->side));
    render_status_bar(screen, &plan->status_bar);
    render_animation_region(screen, &plan->animation);
}
