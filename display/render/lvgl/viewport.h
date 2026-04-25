/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

#include <display/core/dual_display_plan.h>

void zmk_dual_display_lvgl_reset_obj(lv_obj_t *obj);

void zmk_dual_display_lvgl_configure_screen(lv_obj_t *screen);

struct zmk_dual_display_rect zmk_dual_display_lvgl_map_rect(
    const struct zmk_dual_display_rect *bounds);

void zmk_dual_display_lvgl_apply_rect(lv_obj_t *obj,
                                      const struct zmk_dual_display_rect *bounds);
