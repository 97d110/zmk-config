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
#include <display/render/theme/dual_display_theme.h>

#define SIM_WIDTH 20
#define SIM_HEIGHT 22
#define SIM_STATUS_HEIGHT 3
#define SIM_LINE_MAX 128

struct sim_state {
    struct zmk_dual_display_state left;
    struct zmk_dual_display_state right;
    struct zmk_dual_display_theme_context left_theme;
    struct zmk_dual_display_theme_context right_theme;
};

static const char *scene_name(enum zmk_dual_display_scene_kind scene) {
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

static const char *energy_name(enum zmk_dual_display_energy_level energy) {
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

static char status_value_char(enum zmk_dual_display_status_slot_value value) {
    switch (value) {
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_0_10:
        return 'l';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_11_50:
        return 'm';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_51_100:
        return 'h';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_0_10_CHARGING:
        return 'L';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_11_50_CHARGING:
        return 'M';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_51_100_CHARGING:
        return 'H';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_SPLIT_CONNECTED:
        return '=';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_SPLIT_DISCONNECTED:
        return '!';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_TRANSPORT_USB:
        return 'U';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_TRANSPORT_BT:
        return 'B';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_TRANSPORT_DISCONNECTED:
        return 'x';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_TYPE:
        return 'T';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_SYMBOL:
        return 'S';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_MOD:
        return 'M';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_CONFIG:
        return 'C';
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_UNKNOWN:
    default:
        return '?';
    }
}

static uint8_t sim_phase_intensity(enum zmk_dual_display_theme_phase phase) {
    switch (phase) {
    case ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_LIGHT:
        return 1;
    case ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_MEDIUM:
        return 2;
    case ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_HIGH:
        return 3;
    case ZMK_DUAL_DISPLAY_THEME_PHASE_TYPING_PEAK:
        return 4;
    case ZMK_DUAL_DISPLAY_THEME_PHASE_DECAY:
        return 1;
    default:
        return 0;
    }
}

static uint8_t sim_energy_intensity(enum zmk_dual_display_energy_level energy) {
    switch (energy) {
    case ZMK_DUAL_DISPLAY_ENERGY_LOW:
        return 1;
    case ZMK_DUAL_DISPLAY_ENERGY_MEDIUM:
        return 2;
    case ZMK_DUAL_DISPLAY_ENERGY_HIGH:
        return 3;
    default:
        return 0;
    }
}

static char theme_fill_char(const struct zmk_dual_display_animation_plan *plan,
                            const struct zmk_dual_display_theme_snapshot *theme, uint8_t row,
                            uint8_t col) {
    switch (theme->scene) {
    case ZMK_DUAL_DISPLAY_SCENE_SLEEP:
        return '#';
    case ZMK_DUAL_DISPLAY_SCENE_LINK_ERROR:
        return (row == 8 && col == 10) ? '!' : ((row + col) % 4 == 0 ? '.' : ' ');
    case ZMK_DUAL_DISPLAY_SCENE_FALLBACK:
        return ((row / 2) + (col / 3)) % 2 == 0 ? '?' : ' ';
    case ZMK_DUAL_DISPLAY_SCENE_NORMAL:
    default:
        if (plan->layer == ZMK_DUAL_DISPLAY_LAYER_SYMBOL && row % 5 == 0) {
            return '=';
        }
        if (plan->layer == ZMK_DUAL_DISPLAY_LAYER_MOD && (row + col) % 11 == 0) {
            return 'o';
        }
        if (plan->layer == ZMK_DUAL_DISPLAY_LAYER_CONFIG && row == col / 2) {
            return '/';
        }

        const uint8_t intensity = sim_phase_intensity(theme->phase);
        if (theme->variant == ZMK_DUAL_DISPLAY_SCENE_VARIANT_PRIMARY) {
            const uint8_t actor_row = (uint8_t)(6 + (theme->frame_tick % 7));
            const uint8_t actor_col = (uint8_t)(8 + (theme->frame_tick % 4));
            if (row == actor_row && col >= actor_col && col < actor_col + 2 + intensity) {
                return intensity > 0 ? '*' : 'O';
            }
            if (intensity > 0 && row + intensity == actor_row && col < actor_col) {
                return '~';
            }
        } else {
            const uint8_t horizon = (uint8_t)(15 - sim_energy_intensity(plan->energy));
            if (row >= horizon) {
                return '_';
            }
            if (row > 9 && col > 5 && col < 14) {
                return intensity > 0 ? '@' : '0';
            }
        }
        return ' ';
    }
}

static void render_screen_preview(const struct zmk_dual_display_screen_plan *plan,
                                  const struct zmk_dual_display_theme_snapshot *theme,
                                  char out[SIM_HEIGHT][SIM_WIDTH + 1]) {
    for (uint8_t y = 0; y < SIM_HEIGHT; y++) {
        for (uint8_t x = 0; x < SIM_WIDTH; x++) {
            out[y][x] = ' ';
        }
        out[y][SIM_WIDTH] = '\0';
    }

    for (uint8_t y = 0; y < SIM_HEIGHT; y++) {
        out[y][0] = '|';
        out[y][SIM_WIDTH - 1] = '|';
    }
    for (uint8_t x = 0; x < SIM_WIDTH; x++) {
        out[0][x] = '-';
        out[SIM_STATUS_HEIGHT][x] = '-';
        out[SIM_HEIGHT - 1][x] = '-';
    }

    const char *side = zmk_dual_display_side_name(plan->side);
    for (uint8_t i = 0; side[i] != '\0' && 2 + i < SIM_WIDTH - 2; i++) {
        out[1][2 + i] = side[i];
    }
    out[1][SIM_WIDTH - 1] = '|';

    for (uint8_t i = 0; i < plan->status_bar.slot_count && i < ZMK_DUAL_DISPLAY_STATUS_SLOT_COUNT;
         i++) {
        const uint8_t x = 3 + i * 6;
        out[2][x] = '[';
        out[2][x + 1] = status_value_char(plan->status_bar.slots[i].value);
        out[2][x + 2] = ']';
    }

    for (uint8_t y = SIM_STATUS_HEIGHT + 1; y < SIM_HEIGHT - 1; y++) {
        for (uint8_t x = 1; x < SIM_WIDTH - 1; x++) {
            out[y][x] = theme_fill_char(&plan->animation, theme, y - SIM_STATUS_HEIGHT - 1,
                                        x - 1);
        }
    }

    if (plan->animation.charging) {
        out[SIM_STATUS_HEIGHT + 2][SIM_WIDTH - 4] = '+';
    }
}

static void print_dual_preview(const struct zmk_dual_display_dual_plan *plan,
                               const struct zmk_dual_display_theme_snapshot *left_theme,
                               const struct zmk_dual_display_theme_snapshot *right_theme) {
    char left[SIM_HEIGHT][SIM_WIDTH + 1];
    char right[SIM_HEIGHT][SIM_WIDTH + 1];

    render_screen_preview(&plan->left, left_theme, left);
    render_screen_preview(&plan->right, right_theme, right);

    printf("\nleft scene=%s phase=%s tick=%u energy=%s charging=%d | right scene=%s phase=%s tick=%u energy=%s charging=%d\n",
           scene_name(plan->left.animation.scene),
           zmk_dual_display_theme_phase_name(left_theme->phase), left_theme->frame_tick,
           energy_name(plan->left.animation.energy), plan->left.animation.charging,
           scene_name(plan->right.animation.scene),
           zmk_dual_display_theme_phase_name(right_theme->phase), right_theme->frame_tick,
           energy_name(plan->right.animation.energy), plan->right.animation.charging);

    for (uint8_t y = 0; y < SIM_HEIGHT; y++) {
        printf("%s  %s\n", left[y], right[y]);
    }
}

static void render_state(struct sim_state *state) {
    struct zmk_dual_display_dual_plan plan;

    zmk_dual_display_build_dual_plan_from_state(&state->left, &state->right, &plan);
    zmk_dual_display_theme_context_observe_plan(&state->left_theme, &plan.left);
    zmk_dual_display_theme_context_observe_plan(&state->right_theme, &plan.right);

    const struct zmk_dual_display_theme_snapshot left_theme =
        zmk_dual_display_theme_context_snapshot(&state->left_theme);
    const struct zmk_dual_display_theme_snapshot right_theme =
        zmk_dual_display_theme_context_snapshot(&state->right_theme);
    print_dual_preview(&plan, &left_theme, &right_theme);
}

static void render_tick(struct sim_state *state, const char *side) {
    struct zmk_dual_display_dual_plan plan;

    zmk_dual_display_build_dual_plan_from_state(&state->left, &state->right, &plan);
    if (side == NULL || strcmp(side, "left") == 0) {
        zmk_dual_display_theme_context_observe_plan(&state->left_theme, &plan.left);
    }
    if (side == NULL || strcmp(side, "right") == 0) {
        zmk_dual_display_theme_context_observe_plan(&state->right_theme, &plan.right);
    }

    const struct zmk_dual_display_theme_snapshot left_theme =
        zmk_dual_display_theme_context_snapshot(&state->left_theme);
    const struct zmk_dual_display_theme_snapshot right_theme =
        zmk_dual_display_theme_context_snapshot(&state->right_theme);
    print_dual_preview(&plan, &left_theme, &right_theme);
}

static struct zmk_dual_display_state *select_side(struct sim_state *state, const char *side) {
    if (strcmp(side, "left") == 0) {
        return &state->left;
    }
    if (strcmp(side, "right") == 0) {
        return &state->right;
    }
    return NULL;
}

static bool parse_bool_token(const char *value, bool *out) {
    if (strcmp(value, "on") == 0 || strcmp(value, "true") == 0 || strcmp(value, "yes") == 0) {
        *out = true;
        return true;
    }
    if (strcmp(value, "off") == 0 || strcmp(value, "false") == 0 || strcmp(value, "no") == 0) {
        *out = false;
        return true;
    }
    return false;
}

static void print_help(void) {
    puts("commands: show, battery <side> <percent> [charging], activity <side> <idle|typing>,");
    puts("          sleep <side> <on|off>, split <side> <unknown|connected|disconnected>,");
    puts("          transport <side> <unknown|usb|bt|disconnected>, layer <side> <0-3>,");
    puts("          tick <count>|<side> <count>, quit");
}

static bool handle_command(struct sim_state *state, char *line, bool auto_render) {
    char *cmd = strtok(line, " \t\r\n");
    char *side = strtok(NULL, " \t\r\n");
    char *value = strtok(NULL, " \t\r\n");
    char *extra = strtok(NULL, " \t\r\n");

    if (cmd == NULL) {
        return true;
    }
    if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
        return false;
    }
    if (strcmp(cmd, "help") == 0) {
        print_help();
        return true;
    }
    if (strcmp(cmd, "show") == 0) {
        render_state(state);
        return true;
    }
    if (strcmp(cmd, "tick") == 0) {
        const char *tick_side = NULL;
        const char *count_token = side;

        if (side != NULL && (strcmp(side, "left") == 0 || strcmp(side, "right") == 0)) {
            tick_side = side;
            count_token = value;
        }
        const int count = count_token == NULL ? 1 : atoi(count_token);
        for (int i = 0; i < count; i++) {
            render_tick(state, tick_side);
        }
        return true;
    }

    struct zmk_dual_display_state *display_state =
        side == NULL ? NULL : select_side(state, side);
    if (display_state == NULL || value == NULL) {
        puts("invalid command");
        print_help();
        return true;
    }

    struct zmk_dual_display_state previous = *display_state;

    if (strcmp(cmd, "battery") == 0) {
        const int percent = atoi(value);
        const bool charging = extra != NULL && strcmp(extra, "charging") == 0;
        display_state->battery =
            zmk_dual_display_battery_bucket_from_percent((int16_t)percent, charging);
    } else if (strcmp(cmd, "activity") == 0) {
        if (strcmp(value, "typing") == 0) {
            display_state->activity = ZMK_DUAL_DISPLAY_ACTIVITY_TYPING;
        } else if (strcmp(value, "idle") == 0) {
            display_state->activity = ZMK_DUAL_DISPLAY_ACTIVITY_IDLE;
        } else {
            puts("activity expects idle or typing");
            return true;
        }
    } else if (strcmp(cmd, "sleep") == 0) {
        bool sleeping = false;
        if (!parse_bool_token(value, &sleeping)) {
            puts("sleep expects on or off");
            return true;
        }
        display_state->activity =
            sleeping ? ZMK_DUAL_DISPLAY_ACTIVITY_SLEEP : ZMK_DUAL_DISPLAY_ACTIVITY_IDLE;
    } else if (strcmp(cmd, "split") == 0) {
        if (strcmp(value, "connected") == 0) {
            display_state->split_link = ZMK_DUAL_DISPLAY_SPLIT_LINK_CONNECTED;
        } else if (strcmp(value, "disconnected") == 0) {
            display_state->split_link = ZMK_DUAL_DISPLAY_SPLIT_LINK_DISCONNECTED;
        } else {
            display_state->split_link = ZMK_DUAL_DISPLAY_SPLIT_LINK_UNKNOWN;
        }
    } else if (strcmp(cmd, "transport") == 0) {
        if (strcmp(value, "usb") == 0) {
            display_state->transport = zmk_dual_display_transport_state_from_flags(true, false);
        } else if (strcmp(value, "bt") == 0) {
            display_state->transport = zmk_dual_display_transport_state_from_flags(false, true);
        } else if (strcmp(value, "disconnected") == 0) {
            display_state->transport = zmk_dual_display_transport_state_from_flags(false, false);
        } else {
            display_state->transport = ZMK_DUAL_DISPLAY_TRANSPORT_UNKNOWN;
        }
    } else if (strcmp(cmd, "layer") == 0) {
        display_state->layer = zmk_dual_display_layer_mode_from_index((uint8_t)atoi(value));
    } else {
        puts("unknown command");
        print_help();
        return true;
    }

    zmk_dual_display_log_state_transition(&previous, display_state);
    if (auto_render) {
        render_state(state);
    }
    return true;
}

int main(int argc, char **argv) {
    struct sim_state state = {0};
    char line[SIM_LINE_MAX];
    const bool batch = argc > 1 && strcmp(argv[1], "--batch") == 0;

    zmk_dual_display_default_state(ZMK_DUAL_DISPLAY_SIDE_LEFT, &state.left);
    zmk_dual_display_default_state(ZMK_DUAL_DISPLAY_SIDE_RIGHT, &state.right);
    zmk_dual_display_theme_context_init(&state.left_theme, ZMK_DUAL_DISPLAY_SIDE_LEFT);
    zmk_dual_display_theme_context_init(&state.right_theme, ZMK_DUAL_DISPLAY_SIDE_RIGHT);

    if (!batch) {
        puts("dual display simulator");
        print_help();
        render_state(&state);
    }

    while (true) {
        if (!batch) {
            fputs("\nsim> ", stdout);
            fflush(stdout);
        }
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;
        }
        if (!handle_command(&state, line, !batch)) {
            break;
        }
    }

    return 0;
}
