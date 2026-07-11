/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <themes/space/v1/scene_recipe.h>

#include <stddef.h>

#include <themes/space/v1/assets.h>

/*
 * Galaxy sheets (129x98) are larger than the 68x146 region and are drawn clipped
 * at a fixed near-static top-left. Region-relative: origin (48,106) minus the
 * sheet origin (55,48) = (-7, 58). The negative x is intentional (clipped).
 */
#define SPACE_V1_GALAXY_X ((int16_t)-7)
#define SPACE_V1_GALAXY_Y ((int16_t)58)

/* Speed-streak overlay anchors (region-relative), from the v13 manifest. */
static const struct {
    int16_t x;
    int16_t y;
    zmk_dual_display_asset_id_t asset;
} SPACE_V1_STREAKS[6] = {
    {46, 33, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_00},
    {66, 106, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_01},
    {36, 43, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_02},
    {58, 76, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_03},
    {33, 29, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_04},
    {25, 27, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_05},
};

/* Twinkle anchors (region-relative top-left). Simplified from the manifest
 * travel paths to fixed midpoints; full sine paths are deferred. */
static const struct {
    int16_t x;
    int16_t y;
} SPACE_V1_TWINKLES[2] = {
    {48, 10},
    {4, 109},
};

static uint8_t loop64(uint16_t tick) {
    return (uint8_t)(tick % 64u);
}

/*
 * Asteroid rotation frame. Progresses faster during high/peak typing. The
 * divisor/position live behind helpers so full manifest motion can replace them
 * later without touching the command model.
 */
static uint8_t asteroid_frame(uint16_t tick, enum zmk_dual_display_animation_phase phase) {
    const uint16_t divisor = (phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_HIGH ||
                              phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_PEAK)
                                 ? 2u
                                 : 4u;
    return (uint8_t)((tick / divisor) % 16u);
}

static void asteroid_position(uint16_t tick, int16_t *x, int16_t *y) {
    /* Simplified deterministic drift + bob; full manifest sine path deferred. */
    *x = (int16_t)(18 + (tick / 8u) % 4u);
    *y = (int16_t)(46 + (tick / 4u) % 8u);
}

static uint8_t streak_count(enum zmk_dual_display_animation_phase phase) {
    switch (phase) {
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_LIGHT:
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_DECAY:
        return 2;
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_MEDIUM:
        return 4;
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_HIGH:
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_PEAK:
        return 6;
    default:
        return 0;
    }
}

static uint8_t twinkle_count(enum zmk_dual_display_animation_phase phase) {
    switch (phase) {
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_MEDIUM:
        return 1;
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_HIGH:
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_PEAK:
        return 2;
    default:
        return 0;
    }
}

static void emit(struct zmk_dual_display_recipe *recipe,
                 enum zmk_dual_display_recipe_command_kind kind,
                 enum zmk_dual_display_recipe_blend blend, zmk_dual_display_asset_id_t asset,
                 zmk_dual_display_point_set_id_t point_set, int16_t x, int16_t y, uint8_t frame,
                 bool clip) {
    const struct zmk_dual_display_recipe_command command = {
        .kind = kind,
        .blend = blend,
        .asset = asset,
        .point_set = point_set,
        .x = x,
        .y = y,
        .frame = frame,
        .clip = clip,
    };
    zmk_dual_display_recipe_push(recipe, &command);
}

void zmk_dual_display_space_v1_build_recipe(
    const struct zmk_dual_display_animation_snapshot *animation,
    const struct zmk_dual_display_animation_plan *plan, struct zmk_dual_display_recipe *out_recipe) {
    if (out_recipe == NULL) {
        return;
    }
    if (animation == NULL || plan == NULL) {
        zmk_dual_display_recipe_init(out_recipe, (struct zmk_dual_display_rect){0});
        return;
    }

    zmk_dual_display_recipe_init(out_recipe, plan->bounds);

    /* Non-normal scenes (ZMK global sleep, link error, fallback) and visual
     * display-sleep (phase sleep within a normal scene) stay minimal: a frozen
     * black region until real error/sleep assets exist. */
    if (animation->scene != ZMK_DUAL_DISPLAY_SCENE_NORMAL ||
        animation->phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_SLEEP) {
        emit(out_recipe, ZMK_DUAL_DISPLAY_RECIPE_CLEAR_REGION, ZMK_DUAL_DISPLAY_BLEND_CLEAR_BLACK,
             ZMK_DUAL_DISPLAY_ASSET_NONE, ZMK_DUAL_DISPLAY_POINT_SET_NONE, 0, 0, 0, false);
        return;
    }

    const uint16_t tick = animation->frame_tick;

    /* Base scene, in the manifest layer order. */
    emit(out_recipe, ZMK_DUAL_DISPLAY_RECIPE_CLEAR_REGION, ZMK_DUAL_DISPLAY_BLEND_CLEAR_BLACK,
         ZMK_DUAL_DISPLAY_ASSET_NONE, ZMK_DUAL_DISPLAY_POINT_SET_NONE, 0, 0, 0, false);
    emit(out_recipe, ZMK_DUAL_DISPLAY_RECIPE_DRAW_POINTS, ZMK_DUAL_DISPLAY_BLEND_OR_WHITE,
         ZMK_DUAL_DISPLAY_ASSET_NONE, ZMK_DUAL_DISPLAY_SPACE_V1_POINTS_FAR_STARS, 0, 0, 0, false);
    emit(out_recipe, ZMK_DUAL_DISPLAY_RECIPE_DRAW_CLIPPED_SPRITE,
         ZMK_DUAL_DISPLAY_BLEND_COPY_WITH_MASK, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_GALAXY_CORE_ARMS,
         ZMK_DUAL_DISPLAY_POINT_SET_NONE, SPACE_V1_GALAXY_X, SPACE_V1_GALAXY_Y, 0, true);
    emit(out_recipe, ZMK_DUAL_DISPLAY_RECIPE_DRAW_CLIPPED_SPRITE, ZMK_DUAL_DISPLAY_BLEND_OR_WHITE,
         ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_GALAXY_EDGE_STARS, ZMK_DUAL_DISPLAY_POINT_SET_NONE,
         SPACE_V1_GALAXY_X, SPACE_V1_GALAXY_Y, loop64(tick), true);
    emit(out_recipe, ZMK_DUAL_DISPLAY_RECIPE_DRAW_POINTS, ZMK_DUAL_DISPLAY_BLEND_OR_WHITE,
         ZMK_DUAL_DISPLAY_ASSET_NONE, ZMK_DUAL_DISPLAY_SPACE_V1_POINTS_MID_STARS, 0, 0, 0, false);

    /* The peripheral variant is a placeholder environment view: no actor or
     * typing effects until a matching environment asset set exists. */
    if (animation->variant == ZMK_DUAL_DISPLAY_SCENE_VARIANT_SECONDARY) {
        return;
    }

    /* Typing effects: twinkles then speed streaks (manifest layer order). */
    const uint8_t twinkles = twinkle_count(animation->phase);
    for (uint8_t i = 0; i < twinkles; i++) {
        emit(out_recipe, ZMK_DUAL_DISPLAY_RECIPE_DRAW_SPRITE, ZMK_DUAL_DISPLAY_BLEND_OR_WHITE,
             ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_TWINKLE_LARGE_GLOW, ZMK_DUAL_DISPLAY_POINT_SET_NONE,
             SPACE_V1_TWINKLES[i].x, SPACE_V1_TWINKLES[i].y, loop64(tick), false);
    }

    const uint8_t streaks = streak_count(animation->phase);
    for (uint8_t i = 0; i < streaks; i++) {
        emit(out_recipe, ZMK_DUAL_DISPLAY_RECIPE_DRAW_SPRITE, ZMK_DUAL_DISPLAY_BLEND_OR_WHITE,
             SPACE_V1_STREAKS[i].asset, ZMK_DUAL_DISPLAY_POINT_SET_NONE, SPACE_V1_STREAKS[i].x,
             SPACE_V1_STREAKS[i].y, 0, false);
    }

    /* Central actor: clear a black border under it, then draw it masked. */
    int16_t ax;
    int16_t ay;
    asteroid_position(tick, &ax, &ay);
    const uint8_t frame = asteroid_frame(tick, animation->phase);
    emit(out_recipe, ZMK_DUAL_DISPLAY_RECIPE_APPLY_CLEARANCE_MASK,
         ZMK_DUAL_DISPLAY_BLEND_CLEAR_BLACK, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_ASTEROID,
         ZMK_DUAL_DISPLAY_POINT_SET_NONE, ax, ay, frame, false);
    emit(out_recipe, ZMK_DUAL_DISPLAY_RECIPE_DRAW_SPRITE_MASKED,
         ZMK_DUAL_DISPLAY_BLEND_COPY_WITH_MASK, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_ASTEROID,
         ZMK_DUAL_DISPLAY_POINT_SET_NONE, ax, ay, frame, false);
}
