/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(zmk_dual_display, CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE_LOG_LEVEL);

#include <display/core/dual_display_plan.h>

static enum zmk_dual_display_side current_firmware_side(void) {
#if IS_ENABLED(CONFIG_BOARD_EYELASH_SOFLE_RIGHT)
    LOG_DBG("firmware side selected from board config: right");
    return ZMK_DUAL_DISPLAY_SIDE_RIGHT;
#elif IS_ENABLED(CONFIG_BOARD_EYELASH_SOFLE_LEFT)
    LOG_DBG("firmware side selected from board config: left");
    return ZMK_DUAL_DISPLAY_SIDE_LEFT;
#else
    LOG_WRN("unknown board side, falling back to left display plan");
    return ZMK_DUAL_DISPLAY_SIDE_LEFT;
#endif
}

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

    LOG_DBG("rendered status slot kind=%d active=%d at %u,%u %ux%u", slot->kind, slot->active,
            (unsigned int)slot->bounds.x, (unsigned int)slot->bounds.y,
            (unsigned int)slot->bounds.width, (unsigned int)slot->bounds.height);
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

    LOG_DBG("rendered status bar at %u,%u %ux%u with %u slots",
            (unsigned int)plan->bounds.x, (unsigned int)plan->bounds.y,
            (unsigned int)plan->bounds.width, (unsigned int)plan->bounds.height,
            (unsigned int)plan->slot_count);
}

static void render_animation_region(lv_obj_t *screen,
                                    const struct zmk_dual_display_animation_plan *plan) {
    add_rect(screen, &plan->bounds, false);

    const uint8_t cue_x = plan->use_alternate_side_variant ? 136 : 18;

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

    LOG_DBG("rendered top-down animation placeholder asset=%d alternate_variant=%d", plan->asset,
            plan->use_alternate_side_variant);
}

static void render_screen_plan(lv_obj_t *screen, const struct zmk_dual_display_screen_plan *plan) {
    LOG_DBG("rendering %s screen plan", zmk_dual_display_side_name(plan->side));
    render_status_bar(screen, &plan->status_bar);
    render_animation_region(screen, &plan->animation);
}

lv_obj_t *zmk_display_status_screen(void) {
    const enum zmk_dual_display_side side = current_firmware_side();
    struct zmk_dual_display_screen_plan plan;

    LOG_DBG("creating dual display status screen");
    zmk_dual_display_build_screen_plan(side, &plan);

    lv_obj_t *screen = lv_obj_create(NULL);
    clear_obj_defaults(screen);
    lv_obj_set_size(screen, ZMK_DUAL_DISPLAY_WIDTH, ZMK_DUAL_DISPLAY_HEIGHT);
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    render_screen_plan(screen, &plan);
    LOG_DBG("created %s dual display status screen", zmk_dual_display_side_name(side));

    return screen;
}
