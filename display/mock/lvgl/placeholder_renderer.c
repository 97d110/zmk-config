/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <display/mock/lvgl/placeholder_renderer.h>

#include <display/log.h>
#include <display/render/lvgl/viewport.h>

static lv_color_t canvas_buf[ZMK_DUAL_DISPLAY_LONG_EDGE * ZMK_DUAL_DISPLAY_SHORT_EDGE];

static lv_obj_t *create_canvas(lv_obj_t *screen) {
    lv_obj_t *canvas = lv_canvas_create(screen);

    if (canvas == NULL) {
        ZMK_DUAL_DISPLAY_LOG_ERR("mock failed to create placeholder canvas");
        return NULL;
    }

    zmk_dual_display_lvgl_reset_obj(canvas);
    lv_canvas_set_buffer(canvas, canvas_buf, ZMK_DUAL_DISPLAY_LONG_EDGE,
                         ZMK_DUAL_DISPLAY_SHORT_EDGE, LV_IMG_CF_TRUE_COLOR);
    lv_obj_align(canvas, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_canvas_fill_bg(canvas, lv_color_white(), LV_OPA_COVER);

    return canvas;
}

static lv_obj_t *add_rect(lv_obj_t *canvas, const struct zmk_dual_display_rect *bounds,
                          bool filled) {
    if (canvas == NULL || bounds == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("mock skipped rectangle for missing input: canvas=%p bounds=%p",
                                 (void *)canvas, (const void *)bounds);
        return NULL;
    }

    const struct zmk_dual_display_rect panel_bounds = zmk_dual_display_lvgl_map_rect(bounds);

    lv_draw_rect_dsc_t rect_dsc;
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.bg_color = lv_color_black();
    rect_dsc.bg_opa = filled ? LV_OPA_COVER : LV_OPA_TRANSP;
    rect_dsc.border_color = lv_color_black();
    rect_dsc.border_opa = LV_OPA_COVER;
    rect_dsc.border_width = filled ? 0 : 1;

    lv_canvas_draw_rect(canvas, panel_bounds.x, panel_bounds.y, panel_bounds.width,
                        panel_bounds.height, &rect_dsc);

    return canvas;
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
    ZMK_DUAL_DISPLAY_LOG_INF("mock rendering status bar bounds=%u,%u %ux%u slots=%u",
                             (unsigned int)plan->bounds.x, (unsigned int)plan->bounds.y,
                             (unsigned int)plan->bounds.width, (unsigned int)plan->bounds.height,
                             (unsigned int)plan->slot_count);
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

static uint8_t activity_intensity(enum zmk_dual_display_activity_bucket activity) {
    switch (activity) {
    case ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_2S:
        return 1;
    case ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_5S:
        return 2;
    case ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_10S:
        return 3;
    case ZMK_DUAL_DISPLAY_ACTIVITY_TYPING_15S:
        return 4;
    default:
        return 0;
    }
}

static uint8_t energy_intensity(enum zmk_dual_display_energy_level energy) {
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

static void render_scene_normal(lv_obj_t *screen,
                                const struct zmk_dual_display_animation_plan *plan) {
    add_rect(screen, &plan->bounds, false);

    const bool secondary_variant = plan->variant == ZMK_DUAL_DISPLAY_SCENE_VARIANT_SECONDARY;
    const uint8_t cue_x = secondary_variant ? plan->bounds.x + plan->bounds.width - 11
                                            : plan->bounds.x + 5;
    const uint8_t motion_band_width = plan->bounds.width - 20;
    const uint8_t center_frame_width = plan->bounds.width - 30;
    const uint8_t base_lower_band_width = plan->bounds.width - 16;
    const uint8_t lower_band_width =
        base_lower_band_width - 2 * (3 - energy_intensity(plan->energy));
    const uint8_t upper_band_height = 4 + activity_intensity(plan->activity) * 2;

    struct zmk_dual_display_rect upper_motion_band = {
        .x = centered_in(plan->bounds.x, plan->bounds.width, motion_band_width),
        .y = plan->bounds.y + 18,
        .width = motion_band_width,
        .height = upper_band_height,
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
        .x = centered_in(plan->bounds.x, plan->bounds.width, lower_band_width),
        .y = plan->bounds.y + plan->bounds.height - 20,
        .width = lower_band_width,
        .height = 4,
    };

    add_rect(screen, &upper_motion_band, true);
    add_rect(screen, &center_frame, false);
    add_rect(screen, &side_cue, true);
    add_rect(screen, &lower_motion_band, true);
}

static void render_scene_sleep(lv_obj_t *screen,
                               const struct zmk_dual_display_animation_plan *plan) {
    add_rect(screen, &plan->bounds, true);
}

static void render_scene_link_error(lv_obj_t *screen,
                                    const struct zmk_dual_display_animation_plan *plan) {
    add_rect(screen, &plan->bounds, false);

    for (uint8_t y = 0; y < plan->bounds.height; y += 4) {
        const uint8_t offset = (y / 4) % 2 == 0 ? 0 : 2;
        for (uint8_t x = offset; x < plan->bounds.width; x += 4) {
            const struct zmk_dual_display_rect dot = {
                .x = plan->bounds.x + x,
                .y = plan->bounds.y + y,
                .width = 1,
                .height = 1,
            };
            add_rect(screen, &dot, true);
        }
    }

    const uint8_t glyph_x = plan->bounds.x + (plan->bounds.width - 3) / 2;
    const uint8_t glyph_y = plan->bounds.y + (plan->bounds.height - 16) / 2;
    const struct zmk_dual_display_rect bang_stem = {
        .x = glyph_x,
        .y = glyph_y,
        .width = 3,
        .height = 10,
    };
    const struct zmk_dual_display_rect bang_dot = {
        .x = glyph_x,
        .y = glyph_y + 13,
        .width = 3,
        .height = 3,
    };
    add_rect(screen, &bang_stem, true);
    add_rect(screen, &bang_dot, true);
}

static void render_scene_fallback(lv_obj_t *screen,
                                  const struct zmk_dual_display_animation_plan *plan) {
    add_rect(screen, &plan->bounds, false);

    const uint8_t cell = 4;
    for (uint8_t y = 0; y < plan->bounds.height; y += cell) {
        for (uint8_t x = 0; x < plan->bounds.width; x += cell) {
            const bool fill = ((x / cell) + (y / cell)) % 2 == 0;
            if (!fill) {
                continue;
            }
            const uint8_t cell_w =
                (uint8_t)(x + cell <= plan->bounds.width ? cell : plan->bounds.width - x);
            const uint8_t cell_h =
                (uint8_t)(y + cell <= plan->bounds.height ? cell : plan->bounds.height - y);
            const struct zmk_dual_display_rect square = {
                .x = plan->bounds.x + x,
                .y = plan->bounds.y + y,
                .width = cell_w,
                .height = cell_h,
            };
            add_rect(screen, &square, true);
        }
    }
}

static void render_charging_overlay(lv_obj_t *screen,
                                    const struct zmk_dual_display_animation_plan *plan) {
    const uint8_t corner_x = plan->bounds.x + plan->bounds.width - 8;
    const uint8_t corner_y = plan->bounds.y + 4;
    const struct zmk_dual_display_rect bolt_top = {
        .x = corner_x + 2,
        .y = corner_y,
        .width = 2,
        .height = 4,
    };
    const struct zmk_dual_display_rect bolt_mid = {
        .x = corner_x,
        .y = corner_y + 4,
        .width = 4,
        .height = 1,
    };
    const struct zmk_dual_display_rect bolt_bottom = {
        .x = corner_x,
        .y = corner_y + 5,
        .width = 2,
        .height = 4,
    };
    add_rect(screen, &bolt_top, true);
    add_rect(screen, &bolt_mid, true);
    add_rect(screen, &bolt_bottom, true);
}

static void log_scene_change_once(const struct zmk_dual_display_animation_plan *plan) {
    static int last_scene_by_variant[2] = {-1, -1};

    const uint8_t idx =
        plan->variant == ZMK_DUAL_DISPLAY_SCENE_VARIANT_SECONDARY ? 1 : 0;
    if ((int)plan->scene == last_scene_by_variant[idx]) {
        return;
    }
    last_scene_by_variant[idx] = (int)plan->scene;

    ZMK_DUAL_DISPLAY_LOG_DBG(
        "mock rendered portrait animation placeholder variant=%d scene=%d activity=%d "
        "energy=%d charging=%d",
        plan->variant, plan->scene, plan->activity, plan->energy, (int)plan->charging);
}

static void render_animation_region(lv_obj_t *screen,
                                    const struct zmk_dual_display_animation_plan *plan) {
    switch (plan->scene) {
    case ZMK_DUAL_DISPLAY_SCENE_SLEEP:
        render_scene_sleep(screen, plan);
        break;
    case ZMK_DUAL_DISPLAY_SCENE_LINK_ERROR:
        render_scene_link_error(screen, plan);
        break;
    case ZMK_DUAL_DISPLAY_SCENE_FALLBACK:
        render_scene_fallback(screen, plan);
        break;
    case ZMK_DUAL_DISPLAY_SCENE_NORMAL:
    default:
        render_scene_normal(screen, plan);
        break;
    }

    if (plan->charging) {
        render_charging_overlay(screen, plan);
    }

    log_scene_change_once(plan);
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

    lv_obj_t *canvas = create_canvas(screen);
    if (canvas == NULL) {
        return;
    }

    render_status_bar(canvas, &plan->status_bar);
    render_animation_region(canvas, &plan->animation);
}
