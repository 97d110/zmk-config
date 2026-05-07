/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>

#include <display/core/dual_display_plan.h>
#include <display/log.h>

#define ZMK_DUAL_DISPLAY_STATUS_SLOT_TOP 3
#define ZMK_DUAL_DISPLAY_STATUS_SLOT_HEIGHT 8
#define ZMK_DUAL_DISPLAY_STATUS_EDGE_MARGIN 4
#define ZMK_DUAL_DISPLAY_STATUS_BATTERY_WIDTH 18
#define ZMK_DUAL_DISPLAY_STATUS_ICON_WIDTH 12

static struct zmk_dual_display_rect rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height) {
    return (struct zmk_dual_display_rect){
        .x = x,
        .y = y,
        .width = width,
        .height = height,
    };
}

static uint8_t centered_status_slot_x(uint8_t width) {
    return (ZMK_DUAL_DISPLAY_WIDTH - width) / 2;
}

static uint8_t trailing_status_slot_x(uint8_t width) {
    return ZMK_DUAL_DISPLAY_WIDTH - ZMK_DUAL_DISPLAY_STATUS_EDGE_MARGIN - width;
}

static enum zmk_dual_display_status_slot_value
battery_status_value(enum zmk_dual_display_battery_bucket battery) {
    switch (battery) {
    case ZMK_DUAL_DISPLAY_BATTERY_0_10:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_0_10;
    case ZMK_DUAL_DISPLAY_BATTERY_11_50:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_11_50;
    case ZMK_DUAL_DISPLAY_BATTERY_51_100:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_51_100;
    case ZMK_DUAL_DISPLAY_BATTERY_0_10_CHARGING:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_0_10_CHARGING;
    case ZMK_DUAL_DISPLAY_BATTERY_11_50_CHARGING:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_11_50_CHARGING;
    case ZMK_DUAL_DISPLAY_BATTERY_51_100_CHARGING:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_51_100_CHARGING;
    case ZMK_DUAL_DISPLAY_BATTERY_UNKNOWN:
    default:
        ZMK_DUAL_DISPLAY_LOG_WRN("using unknown battery status value for bucket=%d", battery);
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_UNKNOWN;
    }
}

static enum zmk_dual_display_status_slot_value
split_status_value(enum zmk_dual_display_split_link_state split_link) {
    switch (split_link) {
    case ZMK_DUAL_DISPLAY_SPLIT_LINK_CONNECTED:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_SPLIT_CONNECTED;
    case ZMK_DUAL_DISPLAY_SPLIT_LINK_DISCONNECTED:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_SPLIT_DISCONNECTED;
    case ZMK_DUAL_DISPLAY_SPLIT_LINK_UNKNOWN:
    default:
        ZMK_DUAL_DISPLAY_LOG_WRN("using unknown split-link status value for state=%d",
                                 split_link);
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_UNKNOWN;
    }
}

static enum zmk_dual_display_status_slot_value
transport_status_value(enum zmk_dual_display_transport_state transport) {
    switch (transport) {
    case ZMK_DUAL_DISPLAY_TRANSPORT_USB:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_TRANSPORT_USB;
    case ZMK_DUAL_DISPLAY_TRANSPORT_BT:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_TRANSPORT_BT;
    case ZMK_DUAL_DISPLAY_TRANSPORT_DISCONNECTED:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_TRANSPORT_DISCONNECTED;
    case ZMK_DUAL_DISPLAY_TRANSPORT_UNKNOWN:
    default:
        ZMK_DUAL_DISPLAY_LOG_WRN("using unknown transport status value for state=%d",
                                 transport);
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_UNKNOWN;
    }
}

static enum zmk_dual_display_status_slot_value
layer_status_value(enum zmk_dual_display_layer_mode layer) {
    switch (layer) {
    case ZMK_DUAL_DISPLAY_LAYER_TYPE:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_TYPE;
    case ZMK_DUAL_DISPLAY_LAYER_SYMBOL:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_SYMBOL;
    case ZMK_DUAL_DISPLAY_LAYER_MOD:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_MOD;
    case ZMK_DUAL_DISPLAY_LAYER_CONFIG:
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_CONFIG;
    case ZMK_DUAL_DISPLAY_LAYER_UNKNOWN:
    default:
        ZMK_DUAL_DISPLAY_LOG_WRN("using unknown layer status value for mode=%d", layer);
        return ZMK_DUAL_DISPLAY_STATUS_VALUE_UNKNOWN;
    }
}

static void build_status_bar_plan(const struct zmk_dual_display_state *state,
                                  struct zmk_dual_display_status_bar_plan *out_plan) {
    out_plan->bounds = rect(0, 0, ZMK_DUAL_DISPLAY_WIDTH, ZMK_DUAL_DISPLAY_STATUS_BAR_HEIGHT);
    out_plan->slot_count = ZMK_DUAL_DISPLAY_STATUS_SLOT_COUNT;

    out_plan->slots[0] = (struct zmk_dual_display_status_slot_plan){
        .kind = ZMK_DUAL_DISPLAY_STATUS_SLOT_BATTERY,
        .value = battery_status_value(state->battery),
        .bounds = rect(ZMK_DUAL_DISPLAY_STATUS_EDGE_MARGIN, ZMK_DUAL_DISPLAY_STATUS_SLOT_TOP,
                       ZMK_DUAL_DISPLAY_STATUS_BATTERY_WIDTH,
                       ZMK_DUAL_DISPLAY_STATUS_SLOT_HEIGHT),
    };

    out_plan->slots[1] = (struct zmk_dual_display_status_slot_plan){
        .kind = ZMK_DUAL_DISPLAY_STATUS_SLOT_SPLIT_LINK,
        .value = split_status_value(state->split_link),
        .bounds = rect(centered_status_slot_x(ZMK_DUAL_DISPLAY_STATUS_ICON_WIDTH),
                       ZMK_DUAL_DISPLAY_STATUS_SLOT_TOP, ZMK_DUAL_DISPLAY_STATUS_ICON_WIDTH,
                       ZMK_DUAL_DISPLAY_STATUS_SLOT_HEIGHT),
    };

    out_plan->slots[2] = (struct zmk_dual_display_status_slot_plan){
        .kind = state->side == ZMK_DUAL_DISPLAY_SIDE_LEFT
                    ? ZMK_DUAL_DISPLAY_STATUS_SLOT_TRANSPORT
                    : ZMK_DUAL_DISPLAY_STATUS_SLOT_LAYER_MODE,
        .value = state->side == ZMK_DUAL_DISPLAY_SIDE_LEFT ? transport_status_value(state->transport)
                                                           : layer_status_value(state->layer),
        .bounds = rect(trailing_status_slot_x(ZMK_DUAL_DISPLAY_STATUS_ICON_WIDTH),
                       ZMK_DUAL_DISPLAY_STATUS_SLOT_TOP, ZMK_DUAL_DISPLAY_STATUS_ICON_WIDTH,
                       ZMK_DUAL_DISPLAY_STATUS_SLOT_HEIGHT),
    };

    ZMK_DUAL_DISPLAY_LOG_DBG("planned %s portrait status bar on short top edge: %u slots",
                             zmk_dual_display_side_name(state->side),
                             (unsigned int)out_plan->slot_count);
}

static bool side_in_range(enum zmk_dual_display_side side) {
    return side == ZMK_DUAL_DISPLAY_SIDE_LEFT || side == ZMK_DUAL_DISPLAY_SIDE_RIGHT;
}

static bool role_in_range(enum zmk_dual_display_role role) {
    return role == ZMK_DUAL_DISPLAY_ROLE_UNKNOWN || role == ZMK_DUAL_DISPLAY_ROLE_CENTRAL ||
           role == ZMK_DUAL_DISPLAY_ROLE_PERIPHERAL;
}

static bool battery_in_range(enum zmk_dual_display_battery_bucket battery) {
    switch (battery) {
    case ZMK_DUAL_DISPLAY_BATTERY_UNKNOWN:
    case ZMK_DUAL_DISPLAY_BATTERY_0_10:
    case ZMK_DUAL_DISPLAY_BATTERY_11_50:
    case ZMK_DUAL_DISPLAY_BATTERY_51_100:
    case ZMK_DUAL_DISPLAY_BATTERY_0_10_CHARGING:
    case ZMK_DUAL_DISPLAY_BATTERY_11_50_CHARGING:
    case ZMK_DUAL_DISPLAY_BATTERY_51_100_CHARGING:
        return true;
    default:
        return false;
    }
}

static bool activity_in_range(enum zmk_dual_display_activity_state activity) {
    switch (activity) {
    case ZMK_DUAL_DISPLAY_ACTIVITY_IDLE:
    case ZMK_DUAL_DISPLAY_ACTIVITY_SLEEP:
    case ZMK_DUAL_DISPLAY_ACTIVITY_TYPING:
        return true;
    default:
        return false;
    }
}

static bool transport_in_range(enum zmk_dual_display_transport_state transport) {
    switch (transport) {
    case ZMK_DUAL_DISPLAY_TRANSPORT_UNKNOWN:
    case ZMK_DUAL_DISPLAY_TRANSPORT_USB:
    case ZMK_DUAL_DISPLAY_TRANSPORT_BT:
    case ZMK_DUAL_DISPLAY_TRANSPORT_DISCONNECTED:
        return true;
    default:
        return false;
    }
}

static bool split_link_in_range(enum zmk_dual_display_split_link_state split_link) {
    switch (split_link) {
    case ZMK_DUAL_DISPLAY_SPLIT_LINK_UNKNOWN:
    case ZMK_DUAL_DISPLAY_SPLIT_LINK_CONNECTED:
    case ZMK_DUAL_DISPLAY_SPLIT_LINK_DISCONNECTED:
        return true;
    default:
        return false;
    }
}

static bool layer_in_range(enum zmk_dual_display_layer_mode layer) {
    switch (layer) {
    case ZMK_DUAL_DISPLAY_LAYER_UNKNOWN:
    case ZMK_DUAL_DISPLAY_LAYER_TYPE:
    case ZMK_DUAL_DISPLAY_LAYER_SYMBOL:
    case ZMK_DUAL_DISPLAY_LAYER_MOD:
    case ZMK_DUAL_DISPLAY_LAYER_CONFIG:
        return true;
    default:
        return false;
    }
}

static bool state_enums_in_range(const struct zmk_dual_display_state *state) {
    return side_in_range(state->side) && role_in_range(state->role) &&
           battery_in_range(state->battery) && activity_in_range(state->activity) &&
           transport_in_range(state->transport) && split_link_in_range(state->split_link) &&
           layer_in_range(state->layer);
}

static enum zmk_dual_display_energy_level
energy_from_battery(enum zmk_dual_display_battery_bucket battery) {
    switch (battery) {
    case ZMK_DUAL_DISPLAY_BATTERY_0_10:
    case ZMK_DUAL_DISPLAY_BATTERY_0_10_CHARGING:
        return ZMK_DUAL_DISPLAY_ENERGY_LOW;
    case ZMK_DUAL_DISPLAY_BATTERY_11_50:
    case ZMK_DUAL_DISPLAY_BATTERY_11_50_CHARGING:
        return ZMK_DUAL_DISPLAY_ENERGY_MEDIUM;
    case ZMK_DUAL_DISPLAY_BATTERY_51_100:
    case ZMK_DUAL_DISPLAY_BATTERY_51_100_CHARGING:
        return ZMK_DUAL_DISPLAY_ENERGY_HIGH;
    case ZMK_DUAL_DISPLAY_BATTERY_UNKNOWN:
    default:
        ZMK_DUAL_DISPLAY_LOG_DBG("energy=UNKNOWN (battery=UNKNOWN)");
        return ZMK_DUAL_DISPLAY_ENERGY_UNKNOWN;
    }
}

static bool charging_from_battery(enum zmk_dual_display_battery_bucket battery) {
    switch (battery) {
    case ZMK_DUAL_DISPLAY_BATTERY_0_10_CHARGING:
    case ZMK_DUAL_DISPLAY_BATTERY_11_50_CHARGING:
    case ZMK_DUAL_DISPLAY_BATTERY_51_100_CHARGING:
        return true;
    default:
        return false;
    }
}

static enum zmk_dual_display_scene_kind
select_scene_kind(const struct zmk_dual_display_state *state) {
    if (state->activity == ZMK_DUAL_DISPLAY_ACTIVITY_SLEEP) {
        ZMK_DUAL_DISPLAY_LOG_DBG("sleep override: scene=SLEEP side=%s",
                                 zmk_dual_display_side_name(state->side));
        return ZMK_DUAL_DISPLAY_SCENE_SLEEP;
    }

    if (state->split_link == ZMK_DUAL_DISPLAY_SPLIT_LINK_DISCONNECTED) {
        ZMK_DUAL_DISPLAY_LOG_DBG("link-error override: scene=LINK_ERROR side=%s",
                                 zmk_dual_display_side_name(state->side));
        return ZMK_DUAL_DISPLAY_SCENE_LINK_ERROR;
    }

    if (!state_enums_in_range(state)) {
        ZMK_DUAL_DISPLAY_LOG_WRN("invalid state enum; using SCENE_FALLBACK");
        return ZMK_DUAL_DISPLAY_SCENE_FALLBACK;
    }

    ZMK_DUAL_DISPLAY_LOG_DBG("scene selection: scene=NORMAL activity=%d layer=%d",
                             state->activity, state->layer);
    return ZMK_DUAL_DISPLAY_SCENE_NORMAL;
}

static void build_animation_plan(const struct zmk_dual_display_state *state,
                                 struct zmk_dual_display_animation_plan *out_plan) {
    out_plan->bounds = rect(0, ZMK_DUAL_DISPLAY_ANIMATION_Y, ZMK_DUAL_DISPLAY_WIDTH,
                            ZMK_DUAL_DISPLAY_ANIMATION_HEIGHT);
    out_plan->variant = state->side == ZMK_DUAL_DISPLAY_SIDE_RIGHT
                            ? ZMK_DUAL_DISPLAY_SCENE_VARIANT_SECONDARY
                            : ZMK_DUAL_DISPLAY_SCENE_VARIANT_PRIMARY;
    out_plan->scene = select_scene_kind(state);
    out_plan->activity = state->activity;
    out_plan->layer = state->layer;

    if (out_plan->scene == ZMK_DUAL_DISPLAY_SCENE_SLEEP ||
        out_plan->scene == ZMK_DUAL_DISPLAY_SCENE_FALLBACK) {
        out_plan->energy = ZMK_DUAL_DISPLAY_ENERGY_UNKNOWN;
        out_plan->charging = false;
    } else {
        out_plan->energy = energy_from_battery(state->battery);
        out_plan->charging = charging_from_battery(state->battery);
    }

    ZMK_DUAL_DISPLAY_LOG_DBG(
        "planned %s animation: variant=%d scene=%d activity=%d layer=%d energy=%d charging=%d "
        "bounds=%ux%u+%u+%u",
        zmk_dual_display_side_name(state->side), out_plan->variant, out_plan->scene,
        out_plan->activity, out_plan->layer, out_plan->energy, (int)out_plan->charging,
        (unsigned int)out_plan->bounds.width, (unsigned int)out_plan->bounds.height,
        (unsigned int)out_plan->bounds.x, (unsigned int)out_plan->bounds.y);
}

void zmk_dual_display_build_screen_plan_from_state(
    const struct zmk_dual_display_state *state, struct zmk_dual_display_screen_plan *out_plan) {
    if (out_plan == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("screen plan output was NULL");
        return;
    }

    struct zmk_dual_display_state default_state;
    if (state == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("screen plan state was NULL; using default left state");
        zmk_dual_display_default_state(ZMK_DUAL_DISPLAY_SIDE_LEFT, &default_state);
        state = &default_state;
    }

    const enum zmk_dual_display_side side = zmk_dual_display_normalize_side(state->side);
    struct zmk_dual_display_state normalized_state = *state;
    normalized_state.side = side;

    out_plan->side = side;
    build_status_bar_plan(&normalized_state, &out_plan->status_bar);
    build_animation_plan(&normalized_state, &out_plan->animation);

    ZMK_DUAL_DISPLAY_LOG_DBG("built %s screen plan", zmk_dual_display_side_name(side));
}

void zmk_dual_display_build_screen_plan(enum zmk_dual_display_side side,
                                        struct zmk_dual_display_screen_plan *out_plan) {
    struct zmk_dual_display_state state;

    zmk_dual_display_default_state(side, &state);
    zmk_dual_display_build_screen_plan_from_state(&state, out_plan);
}

void zmk_dual_display_build_dual_plan_from_state(
    const struct zmk_dual_display_state *left_state,
    const struct zmk_dual_display_state *right_state, struct zmk_dual_display_dual_plan *out_plan) {
    if (out_plan == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("dual plan output was NULL");
        return;
    }

    struct zmk_dual_display_state default_left_state;
    struct zmk_dual_display_state default_right_state;

    if (left_state == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("left dual plan state was NULL; using default left state");
        zmk_dual_display_default_state(ZMK_DUAL_DISPLAY_SIDE_LEFT, &default_left_state);
        left_state = &default_left_state;
    }

    if (right_state == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("right dual plan state was NULL; using default right state");
        zmk_dual_display_default_state(ZMK_DUAL_DISPLAY_SIDE_RIGHT, &default_right_state);
        right_state = &default_right_state;
    }

    zmk_dual_display_build_screen_plan_from_state(left_state, &out_plan->left);
    zmk_dual_display_build_screen_plan_from_state(right_state, &out_plan->right);

    ZMK_DUAL_DISPLAY_LOG_DBG("built state-aware dual screen plan");
}

void zmk_dual_display_build_dual_plan(struct zmk_dual_display_dual_plan *out_plan) {
    if (out_plan == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("dual plan output was NULL");
        return;
    }

    zmk_dual_display_build_dual_plan_from_state(NULL, NULL, out_plan);

    ZMK_DUAL_DISPLAY_LOG_DBG("built dual screen plan");
}
