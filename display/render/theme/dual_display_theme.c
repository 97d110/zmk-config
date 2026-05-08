/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <display/render/theme/dual_display_theme.h>

#include <stddef.h>

#include <display/log.h>

#define THEME_DECAY_TICKS 3

static bool snapshot_visuals_equal(const struct zmk_dual_display_theme_snapshot *left,
                                   const struct zmk_dual_display_theme_snapshot *right) {
    return left->side == right->side && left->variant == right->variant &&
           left->scene == right->scene && left->activity == right->activity &&
           left->layer == right->layer && left->energy == right->energy &&
           left->phase == right->phase && left->charging == right->charging;
}

static enum zmk_dual_display_theme_phase typing_phase_from_ticks(uint8_t typing_ticks) {
    if (typing_ticks >= 4) {
        return ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_PEAK;
    }
    if (typing_ticks == 3) {
        return ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_HIGH;
    }
    if (typing_ticks == 2) {
        return ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_MEDIUM;
    }
    return ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_LIGHT;
}

static bool phase_is_typing(enum zmk_dual_display_theme_phase phase) {
    return phase == ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_LIGHT ||
           phase == ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_MEDIUM ||
           phase == ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_HIGH ||
           phase == ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_PEAK;
}

static uint32_t next_delay_for_phase(enum zmk_dual_display_theme_phase phase) {
    switch (phase) {
    case ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_LIGHT:
    case ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_MEDIUM:
    case ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_HIGH:
    case ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_PEAK:
    case ZMK_DUAL_DISPLAY_THEME_PHASE_DECAY:
        return ZMK_DUAL_DISPLAY_THEME_DEFAULT_FRAME_MS;
    default:
        return 0;
    }
}

void zmk_dual_display_theme_context_init(struct zmk_dual_display_theme_context *context,
                                         enum zmk_dual_display_side side) {
    if (context == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("ignored theme context init with NULL context");
        return;
    }

    *context = (struct zmk_dual_display_theme_context){
        .snapshot =
            {
                .side = zmk_dual_display_normalize_side(side),
                .variant = side == ZMK_DUAL_DISPLAY_SIDE_RIGHT
                               ? ZMK_DUAL_DISPLAY_SCENE_VARIANT_SECONDARY
                               : ZMK_DUAL_DISPLAY_SCENE_VARIANT_PRIMARY,
                .scene = ZMK_DUAL_DISPLAY_SCENE_NORMAL,
                .previous_scene = ZMK_DUAL_DISPLAY_SCENE_NORMAL,
                .activity = ZMK_DUAL_DISPLAY_ACTIVITY_IDLE,
                .layer = ZMK_DUAL_DISPLAY_LAYER_TYPE,
                .energy = ZMK_DUAL_DISPLAY_ENERGY_UNKNOWN,
                .phase = ZMK_DUAL_DISPLAY_THEME_PHASE_IDLE,
                .next_delay_ms = 0,
            },
        .initialized = true,
    };
}

void zmk_dual_display_theme_context_observe_plan(
    struct zmk_dual_display_theme_context *context,
    const struct zmk_dual_display_screen_plan *plan) {
    if (context == NULL || plan == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("ignored theme plan observation: context=%p plan=%p",
                                 (void *)context, (const void *)plan);
        return;
    }

    if (!context->initialized || context->snapshot.side != plan->side) {
        zmk_dual_display_theme_context_init(context, plan->side);
    }

    const struct zmk_dual_display_theme_snapshot previous = context->snapshot;
    struct zmk_dual_display_theme_snapshot next = previous;
    const bool scene_changed = previous.scene != plan->animation.scene;

    next.side = plan->side;
    next.variant = plan->animation.variant;
    next.previous_scene = previous.scene;
    next.scene = plan->animation.scene;
    next.activity = plan->animation.activity;
    next.layer = plan->animation.layer;
    next.energy = plan->animation.energy;
    next.charging = plan->animation.charging;
    next.frame_tick = (uint16_t)(previous.frame_tick + 1);

    if (scene_changed) {
        next.typing_ticks = 0;
        next.decay_ticks = 0;
    }

    switch (next.scene) {
    case ZMK_DUAL_DISPLAY_SCENE_SLEEP:
        next.phase = ZMK_DUAL_DISPLAY_THEME_PHASE_SLEEP;
        next.typing_ticks = 0;
        next.decay_ticks = 0;
        break;
    case ZMK_DUAL_DISPLAY_SCENE_LINK_ERROR:
        next.phase = ZMK_DUAL_DISPLAY_THEME_PHASE_LINK_ERROR;
        next.typing_ticks = 0;
        next.decay_ticks = 0;
        break;
    case ZMK_DUAL_DISPLAY_SCENE_FALLBACK:
        next.phase = ZMK_DUAL_DISPLAY_THEME_PHASE_FALLBACK;
        next.typing_ticks = 0;
        next.decay_ticks = 0;
        break;
    case ZMK_DUAL_DISPLAY_SCENE_NORMAL:
    default:
        if (next.activity == ZMK_DUAL_DISPLAY_ACTIVITY_TYPING) {
            if (!phase_is_typing(previous.phase)) {
                next.typing_ticks = 1;
            } else if (next.typing_ticks < UINT8_MAX) {
                next.typing_ticks++;
            }
            next.decay_ticks = 0;
            next.phase = typing_phase_from_ticks(next.typing_ticks);
        } else if (phase_is_typing(previous.phase) ||
                   previous.phase == ZMK_DUAL_DISPLAY_THEME_PHASE_DECAY) {
            next.typing_ticks = 0;
            if (next.decay_ticks < UINT8_MAX) {
                next.decay_ticks++;
            }
            next.phase = next.decay_ticks <= THEME_DECAY_TICKS
                             ? ZMK_DUAL_DISPLAY_THEME_PHASE_DECAY
                             : ZMK_DUAL_DISPLAY_THEME_PHASE_IDLE;
        } else {
            next.typing_ticks = 0;
            next.decay_ticks = 0;
            next.phase = ZMK_DUAL_DISPLAY_THEME_PHASE_IDLE;
        }
        break;
    }

    next.next_delay_ms = next_delay_for_phase(next.phase);
    next.wants_next_frame = next.next_delay_ms > 0;
    context->snapshot = next;

    if (scene_changed) {
        ZMK_DUAL_DISPLAY_LOG_DBG("theme scene entry: side=%s scene=%s previous=%s",
                                 zmk_dual_display_side_name(next.side),
                                 zmk_dual_display_theme_scene_name(next.scene),
                                 zmk_dual_display_theme_scene_name(previous.scene));
    }

    if (!snapshot_visuals_equal(&previous, &next)) {
        ZMK_DUAL_DISPLAY_LOG_DBG(
            "theme context changed: side=%s variant=%s phase=%s activity=%d layer=%s energy=%s charging=%d",
            zmk_dual_display_side_name(next.side),
            zmk_dual_display_theme_variant_name(next.variant),
            zmk_dual_display_theme_phase_name(next.phase), next.activity,
            zmk_dual_display_theme_layer_name(next.layer),
            zmk_dual_display_theme_energy_name(next.energy), (int)next.charging);
    }
}

struct zmk_dual_display_theme_snapshot zmk_dual_display_theme_context_snapshot(
    const struct zmk_dual_display_theme_context *context) {
    if (context == NULL) {
        return (struct zmk_dual_display_theme_snapshot){
            .side = ZMK_DUAL_DISPLAY_SIDE_LEFT,
            .variant = ZMK_DUAL_DISPLAY_SCENE_VARIANT_PRIMARY,
            .scene = ZMK_DUAL_DISPLAY_SCENE_FALLBACK,
            .previous_scene = ZMK_DUAL_DISPLAY_SCENE_FALLBACK,
            .activity = ZMK_DUAL_DISPLAY_ACTIVITY_IDLE,
            .layer = ZMK_DUAL_DISPLAY_LAYER_UNKNOWN,
            .energy = ZMK_DUAL_DISPLAY_ENERGY_UNKNOWN,
            .phase = ZMK_DUAL_DISPLAY_THEME_PHASE_FALLBACK,
        };
    }

    return context->snapshot;
}

const char *zmk_dual_display_theme_scene_name(enum zmk_dual_display_scene_kind scene) {
    switch (scene) {
    case ZMK_DUAL_DISPLAY_SCENE_NORMAL:
        return "normal";
    case ZMK_DUAL_DISPLAY_SCENE_SLEEP:
        return "sleep";
    case ZMK_DUAL_DISPLAY_SCENE_LINK_ERROR:
        return "link-error";
    case ZMK_DUAL_DISPLAY_SCENE_FALLBACK:
        return "fallback";
    default:
        return "unknown";
    }
}

const char *zmk_dual_display_theme_phase_name(enum zmk_dual_display_theme_phase phase) {
    switch (phase) {
    case ZMK_DUAL_DISPLAY_THEME_PHASE_IDLE:
        return "idle";
    case ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_LIGHT:
        return "typing-light";
    case ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_MEDIUM:
        return "typing-medium";
    case ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_HIGH:
        return "typing-high";
    case ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_PEAK:
        return "typing-peak";
    case ZMK_DUAL_DISPLAY_THEME_PHASE_DECAY:
        return "decay";
    case ZMK_DUAL_DISPLAY_THEME_PHASE_SLEEP:
        return "sleep";
    case ZMK_DUAL_DISPLAY_THEME_PHASE_LINK_ERROR:
        return "link-error";
    case ZMK_DUAL_DISPLAY_THEME_PHASE_FALLBACK:
        return "fallback";
    default:
        return "unknown";
    }
}

const char *zmk_dual_display_theme_layer_name(enum zmk_dual_display_layer_mode layer) {
    switch (layer) {
    case ZMK_DUAL_DISPLAY_LAYER_TYPE:
        return "type";
    case ZMK_DUAL_DISPLAY_LAYER_SYMBOL:
        return "symbol";
    case ZMK_DUAL_DISPLAY_LAYER_MOD:
        return "mod";
    case ZMK_DUAL_DISPLAY_LAYER_CONFIG:
        return "config";
    case ZMK_DUAL_DISPLAY_LAYER_UNKNOWN:
    default:
        return "unknown";
    }
}

const char *zmk_dual_display_theme_energy_name(enum zmk_dual_display_energy_level energy) {
    switch (energy) {
    case ZMK_DUAL_DISPLAY_ENERGY_LOW:
        return "low";
    case ZMK_DUAL_DISPLAY_ENERGY_MEDIUM:
        return "medium";
    case ZMK_DUAL_DISPLAY_ENERGY_HIGH:
        return "high";
    case ZMK_DUAL_DISPLAY_ENERGY_UNKNOWN:
    default:
        return "unknown";
    }
}

const char *zmk_dual_display_theme_variant_name(enum zmk_dual_display_scene_variant variant) {
    switch (variant) {
    case ZMK_DUAL_DISPLAY_SCENE_VARIANT_PRIMARY:
        return "primary";
    case ZMK_DUAL_DISPLAY_SCENE_VARIANT_SECONDARY:
        return "secondary";
    default:
        return "unknown";
    }
}
