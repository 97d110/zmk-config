/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

#include <display/core/dual_display_plan.h>

/*
 * Temporary proof-of-concept renderer.
 *
 * This adapter intentionally owns the hard-coded placeholder geometry. The
 * permanent LVGL renderer should keep consuming the core screen plan, but this
 * file can be replaced or deleted when real assets and animation playback are
 * introduced.
 */
void zmk_dual_display_mock_lvgl_render_screen_plan(
    lv_obj_t *screen, const struct zmk_dual_display_screen_plan *plan);
