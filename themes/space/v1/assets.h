/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <display/render/recipe/dual_display_recipe.h>

/*
 * Space theme, v1 asset vocabulary.
 *
 * These are the opaque integer asset IDs the space theme's scene-recipe planner
 * emits into recipe commands. The generic recipe layer treats them as opaque
 * uint16_t; only this theme's asset backend (mock in 8D, generated registry
 * later) resolves them to 1-bit pixels. IDs start at 1 so 0 stays "none".
 */

enum zmk_dual_display_space_v1_asset {
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_ASTEROID = 1,
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1,
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_2,
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_PLUS_SMALL,
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_TWINKLE_LARGE_GLOW,
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_00,
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_01,
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_02,
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_03,
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_04,
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_05,
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_GALAXY_CORE_ARMS,
    ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_GALAXY_EDGE_STARS,
};

enum zmk_dual_display_space_v1_point_set {
    ZMK_DUAL_DISPLAY_SPACE_V1_POINTS_FAR_STARS = 1,
    ZMK_DUAL_DISPLAY_SPACE_V1_POINTS_MID_STARS,
};
