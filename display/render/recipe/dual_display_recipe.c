/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <display/render/recipe/dual_display_recipe.h>

#include <stddef.h>

#include <display/log.h>

void zmk_dual_display_recipe_init(struct zmk_dual_display_recipe *recipe,
                                  struct zmk_dual_display_rect region) {
    if (recipe == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("ignored recipe init with NULL recipe");
        return;
    }

    recipe->region = region;
    recipe->command_count = 0;
}

bool zmk_dual_display_recipe_push(struct zmk_dual_display_recipe *recipe,
                                  const struct zmk_dual_display_recipe_command *command) {
    if (recipe == NULL || command == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("ignored recipe push: recipe=%p command=%p", (void *)recipe,
                                 (const void *)command);
        return false;
    }

    if (recipe->command_count >= ZMK_DUAL_DISPLAY_RECIPE_MAX_COMMANDS) {
        ZMK_DUAL_DISPLAY_LOG_WRN("recipe full: dropped command kind=%d (max=%d)",
                                 (int)command->kind, ZMK_DUAL_DISPLAY_RECIPE_MAX_COMMANDS);
        return false;
    }

    recipe->commands[recipe->command_count++] = *command;
    return true;
}

const char *
zmk_dual_display_recipe_command_kind_name(enum zmk_dual_display_recipe_command_kind kind) {
    switch (kind) {
    case ZMK_DUAL_DISPLAY_RECIPE_CLEAR_REGION:
        return "clear_region";
    case ZMK_DUAL_DISPLAY_RECIPE_DRAW_POINTS:
        return "draw_points";
    case ZMK_DUAL_DISPLAY_RECIPE_DRAW_SPRITE:
        return "draw_sprite";
    case ZMK_DUAL_DISPLAY_RECIPE_DRAW_SPRITE_MASKED:
        return "draw_sprite_masked";
    case ZMK_DUAL_DISPLAY_RECIPE_APPLY_CLEARANCE_MASK:
        return "apply_clearance_mask";
    case ZMK_DUAL_DISPLAY_RECIPE_DRAW_CLIPPED_SPRITE:
        return "draw_clipped_sprite";
    default:
        return "unknown";
    }
}

const char *zmk_dual_display_recipe_blend_name(enum zmk_dual_display_recipe_blend blend) {
    switch (blend) {
    case ZMK_DUAL_DISPLAY_BLEND_COPY_WITH_MASK:
        return "copy_with_mask";
    case ZMK_DUAL_DISPLAY_BLEND_OR_WHITE:
        return "or_white";
    case ZMK_DUAL_DISPLAY_BLEND_CLEAR_BLACK:
        return "clear_black";
    case ZMK_DUAL_DISPLAY_BLEND_INVERT_REGION:
        return "invert_region";
    default:
        return "unknown";
    }
}
