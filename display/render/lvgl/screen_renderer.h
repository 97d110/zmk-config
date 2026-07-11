/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

#include <display/core/dual_display_plan.h>
#include <display/render/animation/dual_display_animation.h>

struct zmk_dual_display_render_result {
    bool wants_next_frame;
    uint32_t next_delay_ms;
    struct zmk_dual_display_animation_snapshot theme;
};

/*
 * Durable LVGL renderer contract.
 *
 * The current implementation lives under display/mock/lvgl/ because it draws
 * temporary placeholder geometry. Real rendering should replace that provider
 * while keeping this entry point stable for the firmware status screen.
 */
struct zmk_dual_display_render_result zmk_dual_display_lvgl_render_screen_plan(
    lv_obj_t *screen, const struct zmk_dual_display_screen_plan *plan);
