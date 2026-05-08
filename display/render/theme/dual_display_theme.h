/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <display/core/dual_display_plan.h>

#define ZMK_DUAL_DISPLAY_THEME_DEFAULT_FRAME_MS 250

enum zmk_dual_display_theme_phase {
    ZMK_DUAL_DISPLAY_THEME_PHASE_IDLE,
    ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_LIGHT,
    ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_MEDIUM,
    ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_HIGH,
    ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_PEAK,
    ZMK_DUAL_DISPLAY_THEME_PHASE_DECAY,
    ZMK_DUAL_DISPLAY_THEME_PHASE_SLEEP,
    ZMK_DUAL_DISPLAY_THEME_PHASE_LINK_ERROR,
    ZMK_DUAL_DISPLAY_THEME_PHASE_FALLBACK,
};

struct zmk_dual_display_theme_snapshot {
    enum zmk_dual_display_side side;
    enum zmk_dual_display_scene_variant variant;
    enum zmk_dual_display_scene_kind scene;
    enum zmk_dual_display_scene_kind previous_scene;
    enum zmk_dual_display_activity_state activity;
    enum zmk_dual_display_layer_mode layer;
    enum zmk_dual_display_energy_level energy;
    enum zmk_dual_display_theme_phase phase;
    bool charging;
    uint16_t frame_tick;
    uint8_t typing_ticks;
    uint8_t decay_ticks;
    bool wants_next_frame;
    uint32_t next_delay_ms;
};

struct zmk_dual_display_theme_context {
    struct zmk_dual_display_theme_snapshot snapshot;
    bool initialized;
    bool logged_active_loop;
};

void zmk_dual_display_theme_context_init(struct zmk_dual_display_theme_context *context,
                                         enum zmk_dual_display_side side);

void zmk_dual_display_theme_context_observe_plan(
    struct zmk_dual_display_theme_context *context,
    const struct zmk_dual_display_screen_plan *plan);

struct zmk_dual_display_theme_snapshot zmk_dual_display_theme_context_snapshot(
    const struct zmk_dual_display_theme_context *context);

const char *zmk_dual_display_theme_scene_name(enum zmk_dual_display_scene_kind scene);
const char *zmk_dual_display_theme_phase_name(enum zmk_dual_display_theme_phase phase);
const char *zmk_dual_display_theme_layer_name(enum zmk_dual_display_layer_mode layer);
const char *zmk_dual_display_theme_energy_name(enum zmk_dual_display_energy_level energy);
const char *zmk_dual_display_theme_variant_name(enum zmk_dual_display_scene_variant variant);
