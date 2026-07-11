/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <display/core/dual_display_plan.h>
#include <display/core/dual_display_state.h>
#include <display/render/animation/dual_display_animation.h>
#include <display/render/recipe/dual_display_recipe.h>
#include <themes/space/v1/scene_recipe.h>

#define HOST_TYPING_ACTIVE_MS 1100

struct host_engine {
    struct zmk_dual_display_state left;
    struct zmk_dual_display_state right;
    struct zmk_dual_display_animation_context left_theme;
    struct zmk_dual_display_animation_context right_theme;
    struct zmk_dual_display_animation_timing_profile timing;
    uint32_t now_ms;
    uint32_t typing_until_ms;
};

static const char *battery_name(enum zmk_dual_display_battery_bucket battery) {
    switch (battery) {
    case ZMK_DUAL_DISPLAY_BATTERY_0_10:
        return "0_10";
    case ZMK_DUAL_DISPLAY_BATTERY_11_50:
        return "11_50";
    case ZMK_DUAL_DISPLAY_BATTERY_51_100:
        return "51_100";
    case ZMK_DUAL_DISPLAY_BATTERY_0_10_CHARGING:
        return "0_10_charging";
    case ZMK_DUAL_DISPLAY_BATTERY_11_50_CHARGING:
        return "11_50_charging";
    case ZMK_DUAL_DISPLAY_BATTERY_51_100_CHARGING:
        return "51_100_charging";
    case ZMK_DUAL_DISPLAY_BATTERY_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *activity_name(enum zmk_dual_display_activity_state activity) {
    switch (activity) {
    case ZMK_DUAL_DISPLAY_ACTIVITY_IDLE:
        return "idle";
    case ZMK_DUAL_DISPLAY_ACTIVITY_SLEEP:
        return "sleep";
    case ZMK_DUAL_DISPLAY_ACTIVITY_TYPING:
        return "typing";
    default:
        return "unknown";
    }
}

static const char *transport_name(enum zmk_dual_display_transport_state transport) {
    switch (transport) {
    case ZMK_DUAL_DISPLAY_TRANSPORT_USB:
        return "usb";
    case ZMK_DUAL_DISPLAY_TRANSPORT_BT:
        return "bt";
    case ZMK_DUAL_DISPLAY_TRANSPORT_DISCONNECTED:
        return "disconnected";
    case ZMK_DUAL_DISPLAY_TRANSPORT_UNKNOWN:
    default:
        return "unknown";
    }
}

static const char *split_name(enum zmk_dual_display_split_link_state split) {
    switch (split) {
    case ZMK_DUAL_DISPLAY_SPLIT_LINK_CONNECTED:
        return "connected";
    case ZMK_DUAL_DISPLAY_SPLIT_LINK_DISCONNECTED:
        return "disconnected";
    case ZMK_DUAL_DISPLAY_SPLIT_LINK_UNKNOWN:
    default:
        return "unknown";
    }
}

static void engine_init(struct host_engine *engine) {
    zmk_dual_display_default_state(ZMK_DUAL_DISPLAY_SIDE_LEFT, &engine->left);
    zmk_dual_display_default_state(ZMK_DUAL_DISPLAY_SIDE_RIGHT, &engine->right);
    engine->left.battery = ZMK_DUAL_DISPLAY_BATTERY_51_100_CHARGING;
    engine->left.transport = ZMK_DUAL_DISPLAY_TRANSPORT_USB;
    engine->right.battery = ZMK_DUAL_DISPLAY_BATTERY_51_100;
    engine->timing = zmk_dual_display_animation_default_timing_profile();
    zmk_dual_display_animation_context_init(&engine->left_theme, ZMK_DUAL_DISPLAY_SIDE_LEFT);
    zmk_dual_display_animation_context_init(&engine->right_theme, ZMK_DUAL_DISPLAY_SIDE_RIGHT);
    zmk_dual_display_animation_context_set_timing_profile(&engine->left_theme, &engine->timing);
    zmk_dual_display_animation_context_set_timing_profile(&engine->right_theme, &engine->timing);
    engine->now_ms = 0;
    engine->typing_until_ms = 0;
}

static void maybe_return_idle(struct host_engine *engine) {
    if (engine->typing_until_ms > 0 && engine->now_ms >= engine->typing_until_ms &&
        engine->left.activity == ZMK_DUAL_DISPLAY_ACTIVITY_TYPING) {
        engine->left.activity = ZMK_DUAL_DISPLAY_ACTIVITY_IDLE;
        engine->typing_until_ms = 0;
    }
}

static void observe(struct host_engine *engine, struct zmk_dual_display_dual_plan *plan,
                    uint32_t elapsed_ms) {
    zmk_dual_display_build_dual_plan_from_state(&engine->left, &engine->right, plan);
    zmk_dual_display_animation_context_observe_plan_elapsed(&engine->left_theme, &plan->left,
                                                        elapsed_ms);
    zmk_dual_display_animation_context_observe_plan_elapsed(&engine->right_theme, &plan->right,
                                                        elapsed_ms);
}

static void print_recipe_json(const struct zmk_dual_display_recipe *recipe) {
    printf("\"recipe\":{\"commandCount\":%u,\"commands\":[", (unsigned int)recipe->command_count);
    for (uint8_t i = 0; i < recipe->command_count; i++) {
        const struct zmk_dual_display_recipe_command *command = &recipe->commands[i];
        printf("%s{\"kind\":\"%s\",\"blend\":\"%s\",\"asset\":%u,\"pointSet\":%u,\"frame\":%u,"
               "\"clip\":%s,\"x\":%d,\"y\":%d}",
               i == 0 ? "" : ",", zmk_dual_display_recipe_command_kind_name(command->kind),
               zmk_dual_display_recipe_blend_name(command->blend), (unsigned int)command->asset,
               (unsigned int)command->point_set, (unsigned int)command->frame,
               command->clip ? "true" : "false", (int)command->x, (int)command->y);
    }
    printf("]}");
}

static void print_side_json(const char *key, const struct zmk_dual_display_state *state,
                            const struct zmk_dual_display_animation_snapshot *theme,
                            const struct zmk_dual_display_animation_plan *animation) {
    struct zmk_dual_display_recipe recipe;
    zmk_dual_display_space_v1_build_recipe(theme, animation, &recipe);

    printf("\"%s\":{\"state\":{\"side\":\"%s\",\"battery\":\"%s\",\"activity\":\"%s\","
           "\"transport\":\"%s\",\"split\":\"%s\",\"layer\":\"%s\"},"
           "\"theme\":{\"variant\":\"%s\",\"scene\":\"%s\",\"phase\":\"%s\","
           "\"energy\":\"%s\",\"charging\":%s,\"frameTick\":%u,\"typingElapsedMs\":%u,"
           "\"decayElapsedMs\":%u,\"idleElapsedMs\":%u,\"loopElapsedMs\":%u,"
           "\"wantsNextFrame\":%s,\"nextDelayMs\":%u},",
           key, zmk_dual_display_side_name(state->side), battery_name(state->battery),
           activity_name(state->activity), transport_name(state->transport),
           split_name(state->split_link), zmk_dual_display_animation_layer_name(state->layer),
           zmk_dual_display_animation_variant_name(theme->variant),
           zmk_dual_display_animation_scene_name(theme->scene),
           zmk_dual_display_animation_phase_name(theme->phase),
           zmk_dual_display_animation_energy_name(theme->energy), theme->charging ? "true" : "false",
           (unsigned int)theme->frame_tick, (unsigned int)theme->typing_elapsed_ms,
           (unsigned int)theme->decay_elapsed_ms, (unsigned int)theme->idle_elapsed_ms,
           (unsigned int)theme->loop_elapsed_ms, theme->wants_next_frame ? "true" : "false",
           (unsigned int)theme->next_delay_ms);
    print_recipe_json(&recipe);
    printf("}");
}

static void print_snapshot(struct host_engine *engine, uint32_t elapsed_ms) {
    maybe_return_idle(engine);

    struct zmk_dual_display_dual_plan plan;
    observe(engine, &plan, elapsed_ms);

    const struct zmk_dual_display_animation_snapshot left_theme =
        zmk_dual_display_animation_context_snapshot(&engine->left_theme);
    const struct zmk_dual_display_animation_snapshot right_theme =
        zmk_dual_display_animation_context_snapshot(&engine->right_theme);

    printf("{\"nowMs\":%u,", (unsigned int)engine->now_ms);
    print_side_json("left", &engine->left, &left_theme, &plan.left.animation);
    printf(",");
    print_side_json("right", &engine->right, &right_theme, &plan.right.animation);
    printf("}\n");
    fflush(stdout);
}

static struct zmk_dual_display_state *state_for_side(struct host_engine *engine,
                                                     const char *side) {
    if (strcmp(side, "right") == 0) {
        return &engine->right;
    }
    return &engine->left;
}

static void apply_key(struct host_engine *engine) {
    engine->left.activity = ZMK_DUAL_DISPLAY_ACTIVITY_TYPING;
    engine->typing_until_ms = engine->now_ms + HOST_TYPING_ACTIVE_MS;
}

static void apply_layer(struct host_engine *engine, const char *raw_layer) {
    char *end = NULL;
    const long layer = strtol(raw_layer, &end, 10);
    if (raw_layer[0] != '\0' && end != raw_layer && *end == '\0') {
        engine->left.layer = zmk_dual_display_layer_mode_from_index((uint8_t)layer);
        engine->right.layer = engine->left.layer;
    }
}

static void apply_battery(struct host_engine *engine, char *side, char *percent, char *charging) {
    if (side == NULL || percent == NULL) {
        return;
    }
    const bool is_charging = charging != NULL && strcmp(charging, "1") == 0;
    state_for_side(engine, side)->battery =
        zmk_dual_display_battery_bucket_from_percent((int16_t)atoi(percent), is_charging);
}

static bool apply_profile_field(struct zmk_dual_display_animation_timing_profile *profile,
                                const char *field, uint32_t value) {
    if (strcmp(field, "frame_ms") == 0) {
        profile->frame_ms = value;
    } else if (strcmp(field, "animation_loop_ms") == 0) {
        profile->animation_loop_ms = value;
    } else if (strcmp(field, "typing_light_ms") == 0) {
        profile->typing_light_ms = value;
    } else if (strcmp(field, "typing_medium_ms") == 0) {
        profile->typing_medium_ms = value;
    } else if (strcmp(field, "typing_high_ms") == 0) {
        profile->typing_high_ms = value;
    } else if (strcmp(field, "typing_peak_ms") == 0) {
        profile->typing_peak_ms = value;
    } else if (strcmp(field, "quiet_before_decay_ms") == 0) {
        profile->quiet_before_decay_ms = value;
    } else if (strcmp(field, "decay_to_medium_ms") == 0) {
        profile->decay_to_medium_ms = value;
    } else if (strcmp(field, "decay_to_light_ms") == 0) {
        profile->decay_to_light_ms = value;
    } else if (strcmp(field, "decay_to_idle_ms") == 0) {
        profile->decay_to_idle_ms = value;
    } else if (strcmp(field, "display_sleep_ms") == 0) {
        profile->display_sleep_ms = value;
    } else {
        return false;
    }

    return true;
}

static void apply_profile(struct host_engine *engine) {
    char *assignment = NULL;
    while ((assignment = strtok(NULL, " \t\r\n")) != NULL) {
        char *equals = strchr(assignment, '=');
        if (equals == NULL) {
            continue;
        }
        *equals = '\0';
        char *end = NULL;
        const unsigned long value = strtoul(equals + 1, &end, 10);
        if (*(equals + 1) == '\0' || end == equals + 1 || *end != '\0') {
            continue;
        }
        (void)apply_profile_field(&engine->timing, assignment, (uint32_t)value);
    }

    zmk_dual_display_animation_context_set_timing_profile(&engine->left_theme, &engine->timing);
    zmk_dual_display_animation_context_set_timing_profile(&engine->right_theme, &engine->timing);
}

static enum zmk_dual_display_transport_state transport_from_name(const char *name) {
    if (strcmp(name, "usb") == 0) {
        return ZMK_DUAL_DISPLAY_TRANSPORT_USB;
    }
    if (strcmp(name, "bt") == 0) {
        return ZMK_DUAL_DISPLAY_TRANSPORT_BT;
    }
    if (strcmp(name, "disconnected") == 0) {
        return ZMK_DUAL_DISPLAY_TRANSPORT_DISCONNECTED;
    }
    return ZMK_DUAL_DISPLAY_TRANSPORT_UNKNOWN;
}

static enum zmk_dual_display_split_link_state split_from_name(const char *name) {
    if (strcmp(name, "connected") == 0) {
        return ZMK_DUAL_DISPLAY_SPLIT_LINK_CONNECTED;
    }
    if (strcmp(name, "disconnected") == 0) {
        return ZMK_DUAL_DISPLAY_SPLIT_LINK_DISCONNECTED;
    }
    return ZMK_DUAL_DISPLAY_SPLIT_LINK_UNKNOWN;
}

static void handle_command(struct host_engine *engine, char *line) {
    char *command = strtok(line, " \t\r\n");
    uint32_t elapsed_ms = 0;
    if (command == NULL) {
        return;
    }

    if (strcmp(command, "tick") == 0) {
        char *amount = strtok(NULL, " \t\r\n");
        if (amount != NULL) {
            elapsed_ms = (uint32_t)strtoul(amount, NULL, 10);
            engine->now_ms += elapsed_ms;
        }
    } else if (strcmp(command, "key") == 0) {
        apply_key(engine);
    } else if (strcmp(command, "layer") == 0) {
        char *layer = strtok(NULL, " \t\r\n");
        if (layer != NULL) {
            apply_layer(engine, layer);
        }
    } else if (strcmp(command, "battery") == 0) {
        char *side = strtok(NULL, " \t\r\n");
        char *percent = strtok(NULL, " \t\r\n");
        char *charging = strtok(NULL, " \t\r\n");
        apply_battery(engine, side, percent, charging);
    } else if (strcmp(command, "transport") == 0) {
        char *name = strtok(NULL, " \t\r\n");
        if (name != NULL) {
            engine->left.transport = transport_from_name(name);
        }
    } else if (strcmp(command, "split") == 0) {
        char *side = strtok(NULL, " \t\r\n");
        char *name = strtok(NULL, " \t\r\n");
        if (side != NULL && name != NULL) {
            state_for_side(engine, side)->split_link = split_from_name(name);
        }
    } else if (strcmp(command, "sleep") == 0) {
        char *side = strtok(NULL, " \t\r\n");
        char *enabled = strtok(NULL, " \t\r\n");
        if (side != NULL && enabled != NULL) {
            state_for_side(engine, side)->activity = strcmp(enabled, "1") == 0
                                                        ? ZMK_DUAL_DISPLAY_ACTIVITY_SLEEP
                                                        : ZMK_DUAL_DISPLAY_ACTIVITY_IDLE;
        }
    } else if (strcmp(command, "profile") == 0) {
        apply_profile(engine);
    }

    print_snapshot(engine, elapsed_ms);
}

int main(void) {
    struct host_engine engine;
    engine_init(&engine);
    print_snapshot(&engine, 0);

    char line[256];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        handle_command(&engine, line);
    }

    return 0;
}
