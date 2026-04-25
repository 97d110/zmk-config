/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <display/mock/lvgl/placeholder_renderer.h>

#include <display/log.h>
#include <display/render/lvgl/viewport.h>

static lv_obj_t *add_rect(lv_obj_t *parent, const struct zmk_dual_display_rect *bounds,
                          bool filled) {
    lv_obj_t *obj = lv_obj_create(parent);

    if (obj == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("mock failed to create placeholder rectangle");
        return NULL;
    }

    zmk_dual_display_lvgl_reset_obj(obj);
    zmk_dual_display_lvgl_apply_rect(obj, bounds);
    lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(obj, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, filled ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);

    return obj;
}

static uint8_t centered_in(uint8_t origin, uint8_t parent_size, uint8_t child_size) {
    if (child_size >= parent_size) {
        return origin;
    }

    return origin + ((parent_size - child_size) / 2);
}

static void add_slash_overlay(lv_obj_t *screen, const struct zmk_dual_display_rect *bounds) {
    const uint8_t steps = bounds->width < bounds->height ? bounds->width : bounds->height;

    for (uint8_t i = 0; i < steps; i += 2) {
        const struct zmk_dual_display_rect slash = {
            .x = bounds->x + bounds->width - 1 - i,
            .y = bounds->y + i,
            .width = 1,
            .height = 2,
        };
        add_rect(screen, &slash, true);
    }

    ZMK_DUAL_DISPLAY_LOG_DBG("mock rendered slash overlay for unknown status value");
}

static void add_glyph_pixel(lv_obj_t *screen, uint8_t x, uint8_t y) {
    const struct zmk_dual_display_rect pixel = {
        .x = x,
        .y = y,
        .width = 1,
        .height = 1,
    };
    add_rect(screen, &pixel, true);
}

static void add_glyph(lv_obj_t *screen, const struct zmk_dual_display_rect *bounds,
                      const uint8_t rows[7]) {
    const uint8_t x = centered_in(bounds->x, bounds->width, 5);
    const uint8_t y = centered_in(bounds->y, bounds->height, 7);

    for (uint8_t row = 0; row < 7; row++) {
        for (uint8_t col = 0; col < 5; col++) {
            if ((rows[row] & (1 << (4 - col))) != 0) {
                add_glyph_pixel(screen, x + col, y + row);
            }
        }
    }
}

static void render_layer_glyph(lv_obj_t *screen, const struct zmk_dual_display_rect *bounds,
                               enum zmk_dual_display_status_slot_value value) {
    static const uint8_t glyph_t[7] = {0x1f, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
    static const uint8_t glyph_s[7] = {0x1f, 0x10, 0x10, 0x1f, 0x01, 0x01, 0x1f};
    static const uint8_t glyph_m[7] = {0x11, 0x1b, 0x15, 0x15, 0x11, 0x11, 0x11};
    static const uint8_t glyph_c[7] = {0x0f, 0x10, 0x10, 0x10, 0x10, 0x10, 0x0f};

    switch (value) {
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_TYPE:
        add_glyph(screen, bounds, glyph_t);
        break;
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_SYMBOL:
        add_glyph(screen, bounds, glyph_s);
        break;
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_MOD:
        add_glyph(screen, bounds, glyph_m);
        break;
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_LAYER_CONFIG:
        add_glyph(screen, bounds, glyph_c);
        break;
    default:
        add_rect(screen, bounds, false);
        add_slash_overlay(screen, bounds);
        break;
    }
}

static uint8_t battery_fill_width(enum zmk_dual_display_status_slot_value value) {
    switch (value) {
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_0_10:
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_0_10_CHARGING:
        return 4;
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_11_50:
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_11_50_CHARGING:
        return 8;
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_51_100:
    case ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_51_100_CHARGING:
        return 12;
    default:
        return 0;
    }
}

static bool battery_is_charging(enum zmk_dual_display_status_slot_value value) {
    return value == ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_0_10_CHARGING ||
           value == ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_11_50_CHARGING ||
           value == ZMK_DUAL_DISPLAY_STATUS_VALUE_BATTERY_51_100_CHARGING;
}

static void render_battery_icon(lv_obj_t *screen,
                                const struct zmk_dual_display_status_slot_plan *slot) {
    add_rect(screen, &slot->bounds, false);

    const uint8_t fill_width = battery_fill_width(slot->value);
    if (fill_width > 0) {
        const struct zmk_dual_display_rect fill = {
            .x = slot->bounds.x + 2,
            .y = slot->bounds.y + 2,
            .width = fill_width,
            .height = slot->bounds.height - 4,
        };
        add_rect(screen, &fill, true);
    }

    const struct zmk_dual_display_rect terminal = {
        .x = slot->bounds.x + slot->bounds.width - 1,
        .y = slot->bounds.y + 2,
        .width = 1,
        .height = slot->bounds.height - 4,
    };
    add_rect(screen, &terminal, true);

    if (battery_is_charging(slot->value)) {
        const struct zmk_dual_display_rect charge_mark = {
            .x = slot->bounds.x + slot->bounds.width - 6,
            .y = slot->bounds.y + 1,
            .width = 2,
            .height = slot->bounds.height - 2,
        };
        add_rect(screen, &charge_mark, true);
    }

    if (slot->value == ZMK_DUAL_DISPLAY_STATUS_VALUE_UNKNOWN) {
        add_slash_overlay(screen, &slot->bounds);
    }
}

static void render_transport_icon(lv_obj_t *screen,
                                  const struct zmk_dual_display_status_slot_plan *slot) {
    add_rect(screen, &slot->bounds, false);

    if (slot->value == ZMK_DUAL_DISPLAY_STATUS_VALUE_TRANSPORT_USB) {
        const struct zmk_dual_display_rect stem = {
            .x = slot->bounds.x + 5,
            .y = slot->bounds.y + 1,
            .width = 2,
            .height = slot->bounds.height - 2,
        };
        const struct zmk_dual_display_rect branch = {
            .x = slot->bounds.x + 2,
            .y = slot->bounds.y + 3,
            .width = slot->bounds.width - 4,
            .height = 1,
        };
        add_rect(screen, &stem, true);
        add_rect(screen, &branch, true);
    } else if (slot->value == ZMK_DUAL_DISPLAY_STATUS_VALUE_TRANSPORT_BT) {
        const struct zmk_dual_display_rect spine = {
            .x = slot->bounds.x + 5,
            .y = slot->bounds.y + 1,
            .width = 1,
            .height = slot->bounds.height - 2,
        };
        const struct zmk_dual_display_rect top = {
            .x = slot->bounds.x + 4,
            .y = slot->bounds.y + 1,
            .width = 4,
            .height = 1,
        };
        const struct zmk_dual_display_rect bottom = {
            .x = slot->bounds.x + 4,
            .y = slot->bounds.y + slot->bounds.height - 2,
            .width = 4,
            .height = 1,
        };
        add_rect(screen, &spine, true);
        add_rect(screen, &top, true);
        add_rect(screen, &bottom, true);
    } else if (slot->value == ZMK_DUAL_DISPLAY_STATUS_VALUE_TRANSPORT_DISCONNECTED) {
        const struct zmk_dual_display_rect gap = {
            .x = slot->bounds.x + 3,
            .y = slot->bounds.y + 3,
            .width = slot->bounds.width - 6,
            .height = 2,
        };
        add_rect(screen, &gap, true);
    } else {
        add_slash_overlay(screen, &slot->bounds);
    }
}

static void render_split_icon(lv_obj_t *screen,
                              const struct zmk_dual_display_status_slot_plan *slot) {
    const struct zmk_dual_display_rect base = {
        .x = slot->bounds.x + 5,
        .y = slot->bounds.y + slot->bounds.height - 2,
        .width = 2,
        .height = 2,
    };
    const struct zmk_dual_display_rect mid = {
        .x = slot->bounds.x + 3,
        .y = slot->bounds.y + 3,
        .width = 6,
        .height = 1,
    };
    const struct zmk_dual_display_rect top = {
        .x = slot->bounds.x + 1,
        .y = slot->bounds.y + 1,
        .width = 10,
        .height = 1,
    };

    add_rect(screen, &slot->bounds, false);
    add_rect(screen, &base, true);
    add_rect(screen, &mid, true);
    add_rect(screen, &top, true);

    if (slot->value != ZMK_DUAL_DISPLAY_STATUS_VALUE_SPLIT_CONNECTED) {
        add_slash_overlay(screen, &slot->bounds);
    }
}

static void render_status_slot(lv_obj_t *screen,
                               const struct zmk_dual_display_status_slot_plan *slot) {
    switch (slot->kind) {
    case ZMK_DUAL_DISPLAY_STATUS_SLOT_BATTERY:
        render_battery_icon(screen, slot);
        break;
    case ZMK_DUAL_DISPLAY_STATUS_SLOT_SPLIT_LINK:
        render_split_icon(screen, slot);
        break;
    case ZMK_DUAL_DISPLAY_STATUS_SLOT_TRANSPORT:
        render_transport_icon(screen, slot);
        break;
    case ZMK_DUAL_DISPLAY_STATUS_SLOT_LAYER_MODE:
        render_layer_glyph(screen, &slot->bounds, slot->value);
        break;
    default:
        add_rect(screen, &slot->bounds, false);
        add_slash_overlay(screen, &slot->bounds);
        break;
    }

    ZMK_DUAL_DISPLAY_LOG_DBG("mock rendered status slot kind=%d value=%d at %u,%u %ux%u",
                             slot->kind, slot->value, (unsigned int)slot->bounds.x,
                             (unsigned int)slot->bounds.y, (unsigned int)slot->bounds.width,
                             (unsigned int)slot->bounds.height);
}

static void render_status_bar(lv_obj_t *screen,
                              const struct zmk_dual_display_status_bar_plan *plan) {
    add_rect(screen, &plan->bounds, false);

    for (uint8_t i = 0; i < plan->slot_count && i < ZMK_DUAL_DISPLAY_STATUS_SLOT_COUNT; i++) {
        render_status_slot(screen, &plan->slots[i]);
    }

    const struct zmk_dual_display_rect divider = {
        .x = plan->bounds.x,
        .y = plan->bounds.y + plan->bounds.height - 1,
        .width = plan->bounds.width,
        .height = 1,
    };
    add_rect(screen, &divider, true);

    ZMK_DUAL_DISPLAY_LOG_DBG("mock rendered status bar at %u,%u %ux%u with %u slots",
                             (unsigned int)plan->bounds.x, (unsigned int)plan->bounds.y,
                             (unsigned int)plan->bounds.width,
                             (unsigned int)plan->bounds.height,
                             (unsigned int)plan->slot_count);
}

static void render_animation_region(lv_obj_t *screen,
                                    const struct zmk_dual_display_animation_plan *plan) {
    add_rect(screen, &plan->bounds, false);

    const bool secondary_variant = plan->variant == ZMK_DUAL_DISPLAY_SCENE_VARIANT_SECONDARY;
    const uint8_t cue_x = secondary_variant ? plan->bounds.x + plan->bounds.width - 11
                                            : plan->bounds.x + 5;
    const uint8_t motion_band_width = plan->bounds.width - 20;
    const uint8_t center_frame_width = plan->bounds.width - 30;
    const uint8_t lower_motion_band_width = plan->bounds.width - 16;

    struct zmk_dual_display_rect upper_motion_band = {
        .x = centered_in(plan->bounds.x, plan->bounds.width, motion_band_width),
        .y = plan->bounds.y + 18,
        .width = motion_band_width,
        .height = 8,
    };
    struct zmk_dual_display_rect center_frame = {
        .x = centered_in(plan->bounds.x, plan->bounds.width, center_frame_width),
        .y = plan->bounds.y + 48,
        .width = center_frame_width,
        .height = 42,
    };
    struct zmk_dual_display_rect side_cue = {
        .x = cue_x,
        .y = plan->bounds.y + 68,
        .width = 6,
        .height = 6,
    };
    struct zmk_dual_display_rect lower_motion_band = {
        .x = centered_in(plan->bounds.x, plan->bounds.width, lower_motion_band_width),
        .y = plan->bounds.y + plan->bounds.height - 20,
        .width = lower_motion_band_width,
        .height = 4,
    };

    add_rect(screen, &upper_motion_band, true);
    add_rect(screen, &center_frame, false);
    add_rect(screen, &side_cue, true);
    add_rect(screen, &lower_motion_band, true);

    ZMK_DUAL_DISPLAY_LOG_DBG("mock rendered portrait animation placeholder variant=%d activity=%d",
                             plan->variant, plan->activity);
}

void zmk_dual_display_lvgl_render_screen_plan(
    lv_obj_t *screen, const struct zmk_dual_display_screen_plan *plan) {
    if (screen == NULL || plan == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("mock skipped render for missing input: screen=%p plan=%p",
                                 (void *)screen, (const void *)plan);
        return;
    }

    ZMK_DUAL_DISPLAY_LOG_DBG("mock rendering %s placeholder screen plan",
                             zmk_dual_display_side_name(plan->side));
    render_status_bar(screen, &plan->status_bar);
    render_animation_region(screen, &plan->animation);
}
