/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <display/core/dual_display_plan.h>
#include <display/render/animation/dual_display_animation_timing.h>

#define ZMK_DUAL_DISPLAY_ANIMATION_DEFAULT_FRAME_MS ZMK_DUAL_DISPLAY_ANIMATION_FRAME_MS

enum zmk_dual_display_animation_phase {
    ZMK_DUAL_DISPLAY_ANIMATION_PHASE_IDLE,
    ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_LIGHT,
    ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_MEDIUM,
    ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_HIGH,
    ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_PEAK,
    ZMK_DUAL_DISPLAY_ANIMATION_PHASE_DECAY,
    ZMK_DUAL_DISPLAY_ANIMATION_PHASE_SLEEP,
    ZMK_DUAL_DISPLAY_ANIMATION_PHASE_LINK_ERROR,
    ZMK_DUAL_DISPLAY_ANIMATION_PHASE_FALLBACK,
};

struct zmk_dual_display_animation_snapshot {
    enum zmk_dual_display_side side;
    enum zmk_dual_display_scene_variant variant;
    enum zmk_dual_display_scene_kind scene;
    enum zmk_dual_display_scene_kind previous_scene;
    enum zmk_dual_display_activity_state activity;
    enum zmk_dual_display_layer_mode layer;
    enum zmk_dual_display_energy_level energy;
    enum zmk_dual_display_animation_phase phase;
    bool charging;
    uint16_t frame_tick;
    uint32_t typing_elapsed_ms;
    uint32_t decay_elapsed_ms;
    uint32_t idle_elapsed_ms;
    uint32_t loop_elapsed_ms;
    bool wants_next_frame;
    uint32_t next_delay_ms;
};

struct zmk_dual_display_animation_timing_profile {
    uint32_t frame_ms;
    uint32_t animation_loop_ms;
    uint32_t typing_light_ms;
    uint32_t typing_medium_ms;
    uint32_t typing_high_ms;
    uint32_t typing_peak_ms;
    uint32_t quiet_before_decay_ms;
    uint32_t decay_to_medium_ms;
    uint32_t decay_to_light_ms;
    uint32_t decay_to_idle_ms;
    uint32_t display_sleep_ms;
};

struct zmk_dual_display_animation_context {
    struct zmk_dual_display_animation_snapshot snapshot;
    struct zmk_dual_display_animation_timing_profile timing;
    bool initialized;
    bool logged_active_loop;
};

struct zmk_dual_display_animation_timing_profile zmk_dual_display_animation_default_timing_profile(void);

void zmk_dual_display_animation_context_init(struct zmk_dual_display_animation_context *context,
                                         enum zmk_dual_display_side side);

void zmk_dual_display_animation_context_set_timing_profile(
    struct zmk_dual_display_animation_context *context,
    const struct zmk_dual_display_animation_timing_profile *profile);

void zmk_dual_display_animation_context_observe_plan(
    struct zmk_dual_display_animation_context *context,
    const struct zmk_dual_display_screen_plan *plan);

void zmk_dual_display_animation_context_observe_plan_elapsed(
    struct zmk_dual_display_animation_context *context,
    const struct zmk_dual_display_screen_plan *plan, uint32_t elapsed_ms);

struct zmk_dual_display_animation_snapshot zmk_dual_display_animation_context_snapshot(
    const struct zmk_dual_display_animation_context *context);

const char *zmk_dual_display_animation_scene_name(enum zmk_dual_display_scene_kind scene);
const char *zmk_dual_display_animation_phase_name(enum zmk_dual_display_animation_phase phase);
const char *zmk_dual_display_animation_layer_name(enum zmk_dual_display_layer_mode layer);
const char *zmk_dual_display_animation_energy_name(enum zmk_dual_display_energy_level energy);
const char *zmk_dual_display_animation_variant_name(enum zmk_dual_display_scene_variant variant);
