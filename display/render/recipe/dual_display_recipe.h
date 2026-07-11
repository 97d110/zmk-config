/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <display/core/dual_display_plan.h>

/*
 * Generic, theme-independent render-recipe command model.
 *
 * A recipe is an ordered, renderer-neutral list of composition commands for the
 * animation region. It is produced by a theme-specific planner (under themes/)
 * and consumed by a renderer/compositor. It stays free of LVGL objects, canvas
 * objects, file paths, and heap ownership.
 *
 * Assets are referenced by OPAQUE integer IDs. This layer never learns what an
 * ID means; a theme's assets.h defines the values and its asset backend resolves
 * them to pixels and point coordinates.
 */

typedef uint16_t zmk_dual_display_asset_id_t;
typedef uint16_t zmk_dual_display_point_set_id_t;

#define ZMK_DUAL_DISPLAY_ASSET_NONE ((zmk_dual_display_asset_id_t)0)
#define ZMK_DUAL_DISPLAY_POINT_SET_NONE ((zmk_dual_display_point_set_id_t)0)

enum zmk_dual_display_recipe_command_kind {
    ZMK_DUAL_DISPLAY_RECIPE_CLEAR_REGION,
    ZMK_DUAL_DISPLAY_RECIPE_DRAW_POINTS,
    ZMK_DUAL_DISPLAY_RECIPE_DRAW_SPRITE,
    ZMK_DUAL_DISPLAY_RECIPE_DRAW_SPRITE_MASKED,
    ZMK_DUAL_DISPLAY_RECIPE_APPLY_CLEARANCE_MASK,
    ZMK_DUAL_DISPLAY_RECIPE_DRAW_CLIPPED_SPRITE,
};

enum zmk_dual_display_recipe_blend {
    ZMK_DUAL_DISPLAY_BLEND_COPY_WITH_MASK,
    ZMK_DUAL_DISPLAY_BLEND_OR_WHITE,
    ZMK_DUAL_DISPLAY_BLEND_CLEAR_BLACK,
    ZMK_DUAL_DISPLAY_BLEND_INVERT_REGION, /* reserved for later glitch/error use */
};

struct zmk_dual_display_recipe_command {
    enum zmk_dual_display_recipe_command_kind kind;
    enum zmk_dual_display_recipe_blend blend;
    zmk_dual_display_asset_id_t asset;         /* sprite/mask commands */
    zmk_dual_display_point_set_id_t point_set; /* draw_points commands */
    int16_t x;                                 /* top-left; signed so clipped sprites can be negative */
    int16_t y;
    uint8_t frame;
    bool clip;
};

#define ZMK_DUAL_DISPLAY_RECIPE_MAX_COMMANDS 32

struct zmk_dual_display_recipe {
    struct zmk_dual_display_rect region;
    uint8_t command_count;
    struct zmk_dual_display_recipe_command commands[ZMK_DUAL_DISPLAY_RECIPE_MAX_COMMANDS];
};

/* Reset a recipe to empty with the given animation-region bounds. */
void zmk_dual_display_recipe_init(struct zmk_dual_display_recipe *recipe,
                                  struct zmk_dual_display_rect region);

/*
 * Append a command. Returns false and drops the command (with a warning) when
 * the recipe is already at ZMK_DUAL_DISPLAY_RECIPE_MAX_COMMANDS.
 */
bool zmk_dual_display_recipe_push(struct zmk_dual_display_recipe *recipe,
                                  const struct zmk_dual_display_recipe_command *command);

const char *
zmk_dual_display_recipe_command_kind_name(enum zmk_dual_display_recipe_command_kind kind);
const char *zmk_dual_display_recipe_blend_name(enum zmk_dual_display_recipe_blend blend);
