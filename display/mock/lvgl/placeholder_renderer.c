/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <display/mock/lvgl/placeholder_renderer.h>

#include <display/log.h>
#include <display/render/lvgl/viewport.h>

static lv_obj_t *add_rect(lv_obj_t *parent, const struct zmk_dual_display_rect *bounds,
                          bool filled) {
    lv_obj_t *obj = lv_obj_create(parent);

    if (obj == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("mock failed to create placeholder rectangle");
        return NULL;
    }

    zmk_dual_display_lvgl_reset_obj(obj);
    zmk_dual_display_lvgl_apply_rect(obj, bounds);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(obj, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, filled ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);

    return obj;
}

static uint8_t centered_in(uint8_t origin, uint8_t parent_size, uint8_t child_size) {
    if (child_size >= parent_size) {
        return origin;
    }

    return origin + ((parent_size - child_size) / 2);
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
        .x = plan->bounds.x,
        .y = plan->bounds.y + plan->bounds.height - 1,
        .width = plan->bounds.width,
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
    const uint8_t cue_x = secondary_variant ? plan->bounds.x + plan->bounds.width - 11
                                            : plan->bounds.x + 5;
    const uint8_t motion_band_width = plan->bounds.width - 20;
    const uint8_t center_frame_width = plan->bounds.width - 30;
    const uint8_t lower_motion_band_width = plan->bounds.width - 16;

    struct zmk_dual_display_rect upper_motion_band = {
        .x = centered_in(plan->bounds.x, plan->bounds.width, motion_band_width),
        .y = plan->bounds.y + 18,
        .width = motion_band_width,
        .height = 8,
    };
    struct zmk_dual_display_rect center_frame = {
        .x = centered_in(plan->bounds.x, plan->bounds.width, center_frame_width),
        .y = plan->bounds.y + 48,
        .width = center_frame_width,
        .height = 42,
    };
    struct zmk_dual_display_rect side_cue = {
        .x = cue_x,
        .y = plan->bounds.y + 68,
        .width = 6,
        .height = 6,
    };
    struct zmk_dual_display_rect lower_motion_band = {
        .x = centered_in(plan->bounds.x, plan->bounds.width, lower_motion_band_width),
        .y = plan->bounds.y + plan->bounds.height - 20,
        .width = lower_motion_band_width,
        .height = 4,
    };

    add_rect(screen, &upper_motion_band, true);
    add_rect(screen, &center_frame, false);
    add_rect(screen, &side_cue, true);
    add_rect(screen, &lower_motion_band, true);

    ZMK_DUAL_DISPLAY_LOG_DBG("mock rendered portrait animation placeholder variant=%d",
                             plan->variant);
}

void zmk_dual_display_lvgl_render_screen_plan(
    lv_obj_t *screen, const struct zmk_dual_display_screen_plan *plan) {
    if (screen == NULL || plan == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("mock skipped render for missing input: screen=%p plan=%p",
                                 (void *)screen, (const void *)plan);
        return;
    }

    ZMK_DUAL_DISPLAY_LOG_DBG("mock rendering %s placeholder screen plan",
                             zmk_dual_display_side_name(plan->side));
    render_status_bar(screen, &plan->status_bar);
    render_animation_region(screen, &plan->animation);
}
