/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <display/render/lvgl/viewport.h>

#include <display/log.h>

void zmk_dual_display_lvgl_reset_obj(lv_obj_t *obj) {
    if (obj == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("cannot reset NULL LVGL object");
        return;
    }

    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, 0, LV_PART_MAIN);
}

void zmk_dual_display_lvgl_configure_screen(lv_obj_t *screen) {
    if (screen == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("cannot configure NULL LVGL screen");
        return;
    }

    zmk_dual_display_lvgl_reset_obj(screen);
    lv_obj_set_size(screen, ZMK_DUAL_DISPLAY_LONG_EDGE, ZMK_DUAL_DISPLAY_SHORT_EDGE);
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
}

/* Map the portrait screen plan onto the nice!view panel's landscape framebuffer. */
struct zmk_dual_display_rect zmk_dual_display_lvgl_map_rect(
    const struct zmk_dual_display_rect *bounds) {
    if (bounds == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("cannot map NULL display bounds");
        return (struct zmk_dual_display_rect){0};
    }

    return (struct zmk_dual_display_rect){
        .x = bounds->y,
        .y = ZMK_DUAL_DISPLAY_WIDTH - bounds->x - bounds->width,
        .width = bounds->height,
        .height = bounds->width,
    };
}

void zmk_dual_display_lvgl_apply_rect(lv_obj_t *obj,
                                      const struct zmk_dual_display_rect *bounds) {
    if (obj == NULL || bounds == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("cannot apply LVGL bounds: obj=%p bounds=%p", (void *)obj,
                                 (const void *)bounds);
        return;
    }

    const struct zmk_dual_display_rect panel_bounds = zmk_dual_display_lvgl_map_rect(bounds);

    lv_obj_set_pos(obj, panel_bounds.x, panel_bounds.y);
    lv_obj_set_size(obj, panel_bounds.width, panel_bounds.height);
}
