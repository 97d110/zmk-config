/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

#include <display/core/dual_display_plan.h>

/*
 * Durable LVGL renderer contract.
 *
 * The current implementation lives under display/mock/lvgl/ because it draws
 * temporary placeholder geometry. Real rendering should replace that provider
 * while keeping this entry point stable for the firmware status screen.
 */
void zmk_dual_display_lvgl_render_screen_plan(
    lv_obj_t *screen, const struct zmk_dual_display_screen_plan *plan);
