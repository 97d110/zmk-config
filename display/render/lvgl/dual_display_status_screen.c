/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(zmk_dual_display, CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE_LOG_LEVEL);

#include <display/core/dual_display_plan.h>
#include <display/mock/lvgl/placeholder_renderer.h>

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

static void render_screen_plan(lv_obj_t *screen, const struct zmk_dual_display_screen_plan *plan) {
    if (screen == NULL || plan == NULL) {
        LOG_WRN("skipping render for missing screen plan input: screen=%p plan=%p",
                (void *)screen, (const void *)plan);
        return;
    }

    LOG_DBG("rendering %s screen plan", zmk_dual_display_side_name(plan->side));
    zmk_dual_display_mock_lvgl_render_screen_plan(screen, plan);
}

lv_obj_t *zmk_display_status_screen(void) {
    const enum zmk_dual_display_side side = current_firmware_side();
    struct zmk_dual_display_screen_plan plan;

    LOG_DBG("creating dual display status screen");
    zmk_dual_display_build_screen_plan(side, &plan);

    lv_obj_t *screen = lv_obj_create(NULL);
    if (screen == NULL) {
        LOG_ERR("failed to create dual display status screen object");
        return NULL;
    }

    clear_obj_defaults(screen);
    lv_obj_set_size(screen, ZMK_DUAL_DISPLAY_LONG_EDGE, ZMK_DUAL_DISPLAY_SHORT_EDGE);
    lv_obj_set_style_bg_color(screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);

    render_screen_plan(screen, &plan);
    LOG_DBG("created %s dual display status screen", zmk_dual_display_side_name(side));

    return screen;
}
