/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

#include <display/core/dual_display_state.h>

lv_obj_t *zmk_display_status_screen(void);

int zmk_dual_display_status_screen_update_from_state(
    const struct zmk_dual_display_state *state);
