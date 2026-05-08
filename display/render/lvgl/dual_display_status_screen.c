/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <errno.h>

#include <lvgl.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(zmk_dual_display, CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE_LOG_LEVEL);

#include <display/core/dual_display_plan.h>
#include <display/firmware/dual_display_state_adapter.h>
#include <display/render/lvgl/screen_renderer.h>
#include <display/render/lvgl/dual_display_status_screen.h>
#include <display/render/lvgl/viewport.h>

static lv_obj_t *status_screen;
static struct zmk_dual_display_render_result last_render_result;

static enum zmk_dual_display_side current_firmware_side(void) {
#if IS_ENABLED(CONFIG_BOARD_EYELASH_SOFLE_RIGHT)
    LOG_INF("dual display firmware side selected from board config: right");
    return ZMK_DUAL_DISPLAY_SIDE_RIGHT;
#elif IS_ENABLED(CONFIG_BOARD_EYELASH_SOFLE_LEFT)
    LOG_INF("dual display firmware side selected from board config: left");
    return ZMK_DUAL_DISPLAY_SIDE_LEFT;
#else
    LOG_WRN("unknown board side, falling back to left display plan");
    return ZMK_DUAL_DISPLAY_SIDE_LEFT;
#endif
}

static int render_state_to_screen(lv_obj_t *screen, const struct zmk_dual_display_state *state,
                                  bool refresh) {
    if (screen == NULL || state == NULL) {
        LOG_WRN("ignored dual display render with missing input: screen=%p state=%p",
                (void *)screen, (const void *)state);
        return -EINVAL;
    }

    struct zmk_dual_display_screen_plan plan;
    zmk_dual_display_build_screen_plan_from_state(state, &plan);
    LOG_DBG("dual display plan built: side=%s status_slots=%u animation=%ux%u+%u+%u",
            zmk_dual_display_side_name(plan.side), (unsigned int)plan.status_bar.slot_count,
            (unsigned int)plan.animation.bounds.width, (unsigned int)plan.animation.bounds.height,
            (unsigned int)plan.animation.bounds.x, (unsigned int)plan.animation.bounds.y);

    if (refresh) {
        lv_obj_clean(screen);
        zmk_dual_display_lvgl_configure_screen(screen);
    }

    LOG_DBG("rendering %s dual display screen plan", zmk_dual_display_side_name(plan.side));
    last_render_result = zmk_dual_display_lvgl_render_screen_plan(screen, &plan);

    return 0;
}

int zmk_dual_display_status_screen_update_from_state(
    const struct zmk_dual_display_state *state) {
    if (status_screen == NULL) {
        LOG_WRN("ignored dual display refresh before status screen creation");
        return -ENODEV;
    }

    LOG_DBG("refreshing dual display status screen from firmware state");
    return render_state_to_screen(status_screen, state, true);
}

bool zmk_dual_display_status_screen_next_frame_delay(uint32_t *out_delay_ms) {
    if (last_render_result.wants_next_frame && out_delay_ms != NULL) {
        *out_delay_ms = last_render_result.next_delay_ms;
    }

    return last_render_result.wants_next_frame;
}

struct zmk_dual_display_render_result zmk_dual_display_status_screen_last_render_result(void) {
    return last_render_result;
}

lv_obj_t *zmk_display_status_screen(void) {
    const enum zmk_dual_display_side side = current_firmware_side();
    struct zmk_dual_display_state state;

    LOG_INF("creating dual display status screen: mock_renderer=%d nice_view_widget_status=%d",
            IS_ENABLED(CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE_MOCK_RENDERER),
            IS_ENABLED(CONFIG_NICE_VIEW_WIDGET_STATUS));
    zmk_dual_display_firmware_init_state(side, &state);
    LOG_INF("dual display firmware state: side=%s role=%d battery=%d activity=%d transport=%d split=%d layer=%d",
            zmk_dual_display_side_name(state.side), state.role, state.battery, state.activity,
            state.transport, state.split_link, state.layer);

    lv_obj_t *screen = lv_obj_create(NULL);
    if (screen == NULL) {
        LOG_ERR("failed to create dual display status screen object");
        return NULL;
    }

    zmk_dual_display_lvgl_configure_screen(screen);

    const int err = render_state_to_screen(screen, &state, false);
    if (err < 0) {
        LOG_ERR("failed to render initial dual display status screen: err=%d", err);
        return NULL;
    }

    status_screen = screen;
    LOG_INF("created %s dual display status screen", zmk_dual_display_side_name(side));
    if (last_render_result.wants_next_frame) {
        zmk_dual_display_firmware_schedule_theme_refresh(last_render_result.next_delay_ms);
    }

    return screen;
}
