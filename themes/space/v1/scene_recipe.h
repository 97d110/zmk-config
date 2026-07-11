/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <display/core/dual_display_plan.h>
#include <display/render/animation/dual_display_animation.h>
#include <display/render/recipe/dual_display_recipe.h>

/*
 * Space theme, v1 scene-recipe planner.
 *
 * Turns the generic animation snapshot plus the animation-region bounds into an
 * ordered recipe of composition commands that reference this theme's opaque
 * asset IDs (see assets.h). Deterministic and renderer-neutral: it emits only
 * recipe commands, never pixels.
 */
void zmk_dual_display_space_v1_build_recipe(
    const struct zmk_dual_display_animation_snapshot *animation,
    const struct zmk_dual_display_animation_plan *plan,
    struct zmk_dual_display_recipe *out_recipe);
