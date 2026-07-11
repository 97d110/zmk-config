/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <display/render/animation/dual_display_animation.h>

#include <stddef.h>

#include <display/log.h>

static bool snapshot_visuals_equal(const struct zmk_dual_display_animation_snapshot *left,
                                   const struct zmk_dual_display_animation_snapshot *right) {
    return left->side == right->side && left->variant == right->variant &&
           left->scene == right->scene && left->activity == right->activity &&
           left->layer == right->layer && left->energy == right->energy &&
           left->phase == right->phase && left->charging == right->charging;
}

static uint32_t saturating_add_u32(uint32_t left, uint32_t right) {
    if (UINT32_MAX - left < right) {
        return UINT32_MAX;
    }

    return left + right;
}

static bool advance_loop(uint32_t *loop_elapsed_ms, uint32_t elapsed_ms, uint32_t loop_ms) {
    if (loop_elapsed_ms == NULL || loop_ms == 0) {
        return true;
    }

    const uint32_t previous = *loop_elapsed_ms;
    const uint32_t total = saturating_add_u32(previous, elapsed_ms);
    const bool crossed = total >= loop_ms;
    *loop_elapsed_ms = total % loop_ms;

    return crossed;
}

static enum zmk_dual_display_animation_phase
typing_phase_from_elapsed(const struct zmk_dual_display_animation_timing_profile *timing,
                          uint32_t typing_elapsed_ms) {
    if (typing_elapsed_ms >= timing->typing_peak_ms) {
        return ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_PEAK;
    }
    if (typing_elapsed_ms >= timing->typing_high_ms) {
        return ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_HIGH;
    }
    if (typing_elapsed_ms >= timing->typing_medium_ms) {
        return ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_MEDIUM;
    }
    return ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_LIGHT;
}

static bool phase_is_typing(enum zmk_dual_display_animation_phase phase) {
    return phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_LIGHT ||
           phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_MEDIUM ||
           phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_HIGH ||
           phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_PEAK;
}

static bool phase_is_typing_or_decay(enum zmk_dual_display_animation_phase phase) {
    return phase_is_typing(phase) || phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_DECAY;
}

static bool wants_frame_for_phase(enum zmk_dual_display_scene_kind scene,
                                  enum zmk_dual_display_animation_phase phase) {
    if (scene != ZMK_DUAL_DISPLAY_SCENE_NORMAL) {
        return false;
    }

    return phase != ZMK_DUAL_DISPLAY_ANIMATION_PHASE_SLEEP &&
           phase != ZMK_DUAL_DISPLAY_ANIMATION_PHASE_LINK_ERROR &&
           phase != ZMK_DUAL_DISPLAY_ANIMATION_PHASE_FALLBACK;
}

struct zmk_dual_display_animation_timing_profile zmk_dual_display_animation_default_timing_profile(void) {
    return (struct zmk_dual_display_animation_timing_profile){
        .frame_ms = ZMK_DUAL_DISPLAY_ANIMATION_FRAME_MS,
        .animation_loop_ms = ZMK_DUAL_DISPLAY_ANIMATION_ANIMATION_LOOP_MS,
        .typing_light_ms = ZMK_DUAL_DISPLAY_ANIMATION_TYPING_LIGHT_MS,
        .typing_medium_ms = ZMK_DUAL_DISPLAY_ANIMATION_TYPING_MEDIUM_MS,
        .typing_high_ms = ZMK_DUAL_DISPLAY_ANIMATION_TYPING_HIGH_MS,
        .typing_peak_ms = ZMK_DUAL_DISPLAY_ANIMATION_TYPING_PEAK_MS,
        .quiet_before_decay_ms = ZMK_DUAL_DISPLAY_ANIMATION_QUIET_BEFORE_DECAY_MS,
        .decay_to_medium_ms = ZMK_DUAL_DISPLAY_ANIMATION_DECAY_TO_MEDIUM_MS,
        .decay_to_light_ms = ZMK_DUAL_DISPLAY_ANIMATION_DECAY_TO_LIGHT_MS,
        .decay_to_idle_ms = ZMK_DUAL_DISPLAY_ANIMATION_DECAY_TO_IDLE_MS,
        .display_sleep_ms = ZMK_DUAL_DISPLAY_ANIMATION_DISPLAY_SLEEP_MS,
    };
}

void zmk_dual_display_animation_context_init(struct zmk_dual_display_animation_context *context,
                                         enum zmk_dual_display_side side) {
    if (context == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("ignored animation context init with NULL context");
        return;
    }

    *context = (struct zmk_dual_display_animation_context){
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
                .phase = ZMK_DUAL_DISPLAY_ANIMATION_PHASE_IDLE,
                .next_delay_ms = 0,
            },
        .timing = zmk_dual_display_animation_default_timing_profile(),
        .initialized = true,
    };
}

void zmk_dual_display_animation_context_set_timing_profile(
    struct zmk_dual_display_animation_context *context,
    const struct zmk_dual_display_animation_timing_profile *profile) {
    if (context == NULL || profile == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("ignored animation timing update: context=%p profile=%p",
                                 (void *)context, (const void *)profile);
        return;
    }

    context->timing = *profile;
    context->snapshot.next_delay_ms =
        context->snapshot.wants_next_frame ? context->timing.frame_ms : 0;
    ZMK_DUAL_DISPLAY_LOG_DBG(
        "animation timing profile updated: frame_ms=%u loop_ms=%u medium_ms=%u high_ms=%u peak_ms=%u sleep_ms=%u",
        (unsigned int)context->timing.frame_ms,
        (unsigned int)context->timing.animation_loop_ms,
        (unsigned int)context->timing.typing_medium_ms,
        (unsigned int)context->timing.typing_high_ms,
        (unsigned int)context->timing.typing_peak_ms,
        (unsigned int)context->timing.display_sleep_ms);
}

void zmk_dual_display_animation_context_observe_plan_elapsed(
    struct zmk_dual_display_animation_context *context,
    const struct zmk_dual_display_screen_plan *plan, uint32_t elapsed_ms) {
    if (context == NULL || plan == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("ignored animation plan observation: context=%p plan=%p",
                                 (void *)context, (const void *)plan);
        return;
    }

    if (!context->initialized || context->snapshot.side != plan->side) {
        zmk_dual_display_animation_context_init(context, plan->side);
    }

    const struct zmk_dual_display_animation_snapshot previous = context->snapshot;
    struct zmk_dual_display_animation_snapshot next = previous;
    const struct zmk_dual_display_animation_timing_profile *timing = &context->timing;
    const bool scene_changed = previous.scene != plan->animation.scene;
    const bool loop_boundary = advance_loop(&next.loop_elapsed_ms, elapsed_ms,
                                            timing->animation_loop_ms);

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
        next.typing_elapsed_ms = 0;
        next.decay_elapsed_ms = 0;
        next.idle_elapsed_ms = 0;
        next.loop_elapsed_ms = 0;
    }

    switch (next.scene) {
    case ZMK_DUAL_DISPLAY_SCENE_SLEEP:
        next.phase = ZMK_DUAL_DISPLAY_ANIMATION_PHASE_SLEEP;
        next.typing_elapsed_ms = 0;
        next.decay_elapsed_ms = 0;
        next.idle_elapsed_ms = 0;
        break;
    case ZMK_DUAL_DISPLAY_SCENE_LINK_ERROR:
        next.phase = ZMK_DUAL_DISPLAY_ANIMATION_PHASE_LINK_ERROR;
        next.typing_elapsed_ms = 0;
        next.decay_elapsed_ms = 0;
        next.idle_elapsed_ms = 0;
        break;
    case ZMK_DUAL_DISPLAY_SCENE_FALLBACK:
        next.phase = ZMK_DUAL_DISPLAY_ANIMATION_PHASE_FALLBACK;
        next.typing_elapsed_ms = 0;
        next.decay_elapsed_ms = 0;
        next.idle_elapsed_ms = 0;
        break;
    case ZMK_DUAL_DISPLAY_SCENE_NORMAL:
    default:
        if (next.activity == ZMK_DUAL_DISPLAY_ACTIVITY_TYPING) {
            if (!phase_is_typing(previous.phase)) {
                next.typing_elapsed_ms = 0;
                next.loop_elapsed_ms = 0;
            } else {
                next.typing_elapsed_ms =
                    saturating_add_u32(previous.typing_elapsed_ms, elapsed_ms);
            }
            next.decay_elapsed_ms = 0;
            next.idle_elapsed_ms = 0;
            next.phase = typing_phase_from_elapsed(timing, next.typing_elapsed_ms);
        } else if (phase_is_typing_or_decay(previous.phase)) {
            next.typing_elapsed_ms = 0;
            next.decay_elapsed_ms =
                saturating_add_u32(previous.decay_elapsed_ms, elapsed_ms);
            next.idle_elapsed_ms = 0;

            const uint32_t decay_to_medium_ms =
                saturating_add_u32(timing->quiet_before_decay_ms, timing->decay_to_medium_ms);
            const uint32_t decay_to_light_ms =
                saturating_add_u32(timing->quiet_before_decay_ms, timing->decay_to_light_ms);
            const uint32_t decay_to_idle_ms =
                saturating_add_u32(timing->quiet_before_decay_ms, timing->decay_to_idle_ms);

            if ((previous.phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_PEAK ||
                 previous.phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_HIGH) &&
                next.decay_elapsed_ms >= decay_to_medium_ms && loop_boundary) {
                next.phase = ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_MEDIUM;
            } else if (previous.phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_MEDIUM &&
                       next.decay_elapsed_ms >= decay_to_light_ms && loop_boundary) {
                next.phase = ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_LIGHT;
            } else if (previous.phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_LIGHT &&
                       next.decay_elapsed_ms >= decay_to_idle_ms && loop_boundary) {
                next.phase = ZMK_DUAL_DISPLAY_ANIMATION_PHASE_IDLE;
                next.decay_elapsed_ms = 0;
                next.idle_elapsed_ms = 0;
            } else {
                next.phase = previous.phase;
            }
        } else if (previous.phase == ZMK_DUAL_DISPLAY_ANIMATION_PHASE_SLEEP) {
            next.typing_elapsed_ms = 0;
            next.decay_elapsed_ms = 0;
            next.phase = ZMK_DUAL_DISPLAY_ANIMATION_PHASE_SLEEP;
        } else {
            next.typing_elapsed_ms = 0;
            next.decay_elapsed_ms = 0;
            next.idle_elapsed_ms = saturating_add_u32(previous.idle_elapsed_ms, elapsed_ms);
            next.phase = next.idle_elapsed_ms >= timing->display_sleep_ms
                             ? ZMK_DUAL_DISPLAY_ANIMATION_PHASE_SLEEP
                             : ZMK_DUAL_DISPLAY_ANIMATION_PHASE_IDLE;
        }
        break;
    }

    next.wants_next_frame = wants_frame_for_phase(next.scene, next.phase);
    next.next_delay_ms = next.wants_next_frame ? timing->frame_ms : 0;
    context->snapshot = next;

    if (scene_changed) {
        ZMK_DUAL_DISPLAY_LOG_DBG("animation scene entry: side=%s scene=%s previous=%s",
                                 zmk_dual_display_side_name(next.side),
                                 zmk_dual_display_animation_scene_name(next.scene),
                                 zmk_dual_display_animation_scene_name(previous.scene));
    }

    if (!snapshot_visuals_equal(&previous, &next)) {
        ZMK_DUAL_DISPLAY_LOG_DBG(
            "animation context changed: side=%s variant=%s phase=%s activity=%d layer=%s energy=%s charging=%d typing_ms=%u decay_ms=%u idle_ms=%u",
            zmk_dual_display_side_name(next.side),
            zmk_dual_display_animation_variant_name(next.variant),
            zmk_dual_display_animation_phase_name(next.phase), next.activity,
            zmk_dual_display_animation_layer_name(next.layer),
            zmk_dual_display_animation_energy_name(next.energy), (int)next.charging,
            (unsigned int)next.typing_elapsed_ms, (unsigned int)next.decay_elapsed_ms,
            (unsigned int)next.idle_elapsed_ms);
    }
}

void zmk_dual_display_animation_context_observe_plan(
    struct zmk_dual_display_animation_context *context,
    const struct zmk_dual_display_screen_plan *plan) {
    const uint32_t elapsed_ms =
        context == NULL ? ZMK_DUAL_DISPLAY_ANIMATION_FRAME_MS : context->timing.frame_ms;
    zmk_dual_display_animation_context_observe_plan_elapsed(context, plan, elapsed_ms);
}

struct zmk_dual_display_animation_snapshot zmk_dual_display_animation_context_snapshot(
    const struct zmk_dual_display_animation_context *context) {
    if (context == NULL) {
        return (struct zmk_dual_display_animation_snapshot){
            .side = ZMK_DUAL_DISPLAY_SIDE_LEFT,
            .variant = ZMK_DUAL_DISPLAY_SCENE_VARIANT_PRIMARY,
            .scene = ZMK_DUAL_DISPLAY_SCENE_FALLBACK,
            .previous_scene = ZMK_DUAL_DISPLAY_SCENE_FALLBACK,
            .activity = ZMK_DUAL_DISPLAY_ACTIVITY_IDLE,
            .layer = ZMK_DUAL_DISPLAY_LAYER_UNKNOWN,
            .energy = ZMK_DUAL_DISPLAY_ENERGY_UNKNOWN,
            .phase = ZMK_DUAL_DISPLAY_ANIMATION_PHASE_FALLBACK,
        };
    }

    return context->snapshot;
}

const char *zmk_dual_display_animation_scene_name(enum zmk_dual_display_scene_kind scene) {
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

const char *zmk_dual_display_animation_phase_name(enum zmk_dual_display_animation_phase phase) {
    switch (phase) {
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_IDLE:
        return "idle";
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_LIGHT:
        return "typing-light";
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_MEDIUM:
        return "typing-medium";
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_HIGH:
        return "typing-high";
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_TYPING_PEAK:
        return "typing-peak";
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_DECAY:
        return "decay";
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_SLEEP:
        return "sleep";
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_LINK_ERROR:
        return "link-error";
    case ZMK_DUAL_DISPLAY_ANIMATION_PHASE_FALLBACK:
        return "fallback";
    default:
        return "unknown";
    }
}

const char *zmk_dual_display_animation_layer_name(enum zmk_dual_display_layer_mode layer) {
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

const char *zmk_dual_display_animation_energy_name(enum zmk_dual_display_energy_level energy) {
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

const char *zmk_dual_display_animation_variant_name(enum zmk_dual_display_scene_variant variant) {
    switch (variant) {
    case ZMK_DUAL_DISPLAY_SCENE_VARIANT_PRIMARY:
        return "primary";
    case ZMK_DUAL_DISPLAY_SCENE_VARIANT_SECONDARY:
        return "secondary";
    default:
        return "unknown";
    }
}
