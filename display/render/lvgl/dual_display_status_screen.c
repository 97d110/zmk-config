/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <lvgl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(zmk_dual_display, CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE_LOG_LEVEL);

#include <display/core/dual_display_plan.h>
#include <display/render/lvgl/screen_renderer.h>
#include <display/render/lvgl/viewport.h>

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

    zmk_dual_display_lvgl_configure_screen(screen);

    LOG_DBG("rendering %s screen plan", zmk_dual_display_side_name(plan.side));
    zmk_dual_display_lvgl_render_screen_plan(screen, &plan);
    LOG_DBG("created %s dual display status screen", zmk_dual_display_side_name(side));

    return screen;
}
