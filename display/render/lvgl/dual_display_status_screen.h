/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

#include <display/core/dual_display_state.h>
#include <display/render/lvgl/screen_renderer.h>

lv_obj_t *zmk_display_status_screen(void);

int zmk_dual_display_status_screen_update_from_state(
    const struct zmk_dual_display_state *state);

bool zmk_dual_display_status_screen_next_frame_delay(uint32_t *out_delay_ms);

struct zmk_dual_display_render_result zmk_dual_display_status_screen_last_render_result(void);
