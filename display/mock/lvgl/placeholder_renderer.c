/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <display/mock/lvgl/placeholder_renderer.h>

#include <display/log.h>
#include <display/render/lvgl/viewport.h>
#include <display/render/theme/dual_display_theme.h>

static lv_color_t canvas_buf[ZMK_DUAL_DISPLAY_LONG_EDGE * ZMK_DUAL_DISPLAY_SHORT_EDGE];
static struct zmk_dual_display_theme_context theme_contexts[2];

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

    const struct zmk_dual_display_rect battery_contact = {
        .x = slot->bounds.x + slot->bounds.width - 1,
        .y = slot->bounds.y + 2,
        .width = 1,
        .height = slot->bounds.height - 4,
    };
    add_rect(screen, &battery_contact, true);

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

static uint8_t phase_intensity(enum zmk_dual_display_theme_phase phase) {
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

static struct zmk_dual_display_theme_context *theme_context_for_side(
    enum zmk_dual_display_side side) {
    return &theme_contexts[side == ZMK_DUAL_DISPLAY_SIDE_RIGHT ? 1 : 0];
}

static void render_starfield(lv_obj_t *screen, const struct zmk_dual_display_rect *bounds,
                             const struct zmk_dual_display_theme_snapshot *theme) {
    const uint8_t stars = 7 + energy_intensity(theme->energy) * 3;
    for (uint8_t i = 0; i < stars; i++) {
        const uint8_t x = (uint8_t)((i * 17 + theme->frame_tick * 3) % bounds->width);
        const uint8_t y = (uint8_t)((i * 23 + theme->frame_tick * 5) % bounds->height);
        const struct zmk_dual_display_rect star = {
            .x = bounds->x + x,
            .y = bounds->y + y,
            .width = 1,
            .height = 1,
        };
        add_rect(screen, &star, true);
    }
}

static void render_layer_modifier(lv_obj_t *screen, const struct zmk_dual_display_rect *bounds,
                                  enum zmk_dual_display_layer_mode layer) {
    switch (layer) {
    case ZMK_DUAL_DISPLAY_LAYER_SYMBOL:
        for (uint8_t y = 12; y < bounds->height; y += 18) {
            const struct zmk_dual_display_rect dash = {
                .x = bounds->x + 6,
                .y = bounds->y + y,
                .width = bounds->width - 12,
                .height = 1,
            };
            add_rect(screen, &dash, true);
        }
        break;
    case ZMK_DUAL_DISPLAY_LAYER_MOD:
        for (uint8_t i = 0; i < 4; i++) {
            const struct zmk_dual_display_rect block = {
                .x = bounds->x + 8 + i * 12,
                .y = bounds->y + 24 + (i % 2) * 28,
                .width = 5,
                .height = 5,
            };
            add_rect(screen, &block, false);
        }
        break;
    case ZMK_DUAL_DISPLAY_LAYER_CONFIG:
        for (uint8_t i = 0; i < bounds->width; i += 6) {
            const struct zmk_dual_display_rect tunnel = {
                .x = bounds->x + i,
                .y = bounds->y + i / 2,
                .width = 1,
                .height = bounds->height - i,
            };
            add_rect(screen, &tunnel, true);
        }
        break;
    default:
        break;
    }
}

static void render_primary_theme(lv_obj_t *screen,
                                 const struct zmk_dual_display_animation_plan *plan,
                                 const struct zmk_dual_display_theme_snapshot *theme) {
    const uint8_t intensity = phase_intensity(theme->phase);
    const uint8_t energy = energy_intensity(theme->energy);
    const uint8_t body = 7 + intensity + energy;
    const uint8_t x = centered_in(plan->bounds.x, plan->bounds.width, body);
    const uint8_t y = plan->bounds.y + 42 + ((theme->frame_tick * 5) % 22);

    for (uint8_t i = 0; i < intensity + energy + 1; i++) {
        const struct zmk_dual_display_rect tail = {
            .x = x > i * 3 ? x - i * 3 : plan->bounds.x,
            .y = y > i * 5 ? y - i * 5 : plan->bounds.y,
            .width = body > i ? body - i : 1,
            .height = 2,
        };
        add_rect(screen, &tail, true);
    }

    const struct zmk_dual_display_rect meteor = {
        .x = x,
        .y = y,
        .width = body,
        .height = body,
    };
    add_rect(screen, &meteor, false);

    if (theme->phase == ZMK_DUAL_DISPLAY_THEME_PHASE_DECAY) {
        const struct zmk_dual_display_rect wake = {
            .x = centered_in(plan->bounds.x, plan->bounds.width, 28),
            .y = y + body + 8,
            .width = 28,
            .height = 1,
        };
        add_rect(screen, &wake, true);
    }
}

static void render_secondary_theme(lv_obj_t *screen,
                                   const struct zmk_dual_display_animation_plan *plan,
                                   const struct zmk_dual_display_theme_snapshot *theme) {
    const uint8_t intensity = phase_intensity(theme->phase);
    const uint8_t energy = energy_intensity(theme->energy);
    const uint8_t horizon_y = plan->bounds.y + plan->bounds.height - 26 - energy * 3;

    const struct zmk_dual_display_rect horizon = {
        .x = plan->bounds.x + 6,
        .y = horizon_y,
        .width = plan->bounds.width - 12,
        .height = 3 + energy,
    };
    add_rect(screen, &horizon, true);

    const uint8_t planet_size = 18 + energy * 4 + intensity;
    const struct zmk_dual_display_rect planet = {
        .x = centered_in(plan->bounds.x, plan->bounds.width, planet_size),
        .y = horizon_y - planet_size + 4,
        .width = planet_size,
        .height = planet_size,
    };
    add_rect(screen, &planet, false);

    if (intensity > 0) {
        const struct zmk_dual_display_rect approach = {
            .x = centered_in(plan->bounds.x, plan->bounds.width, 8 + intensity * 4),
            .y = plan->bounds.y + 18 + ((theme->frame_tick * 4) % 30),
            .width = 8 + intensity * 4,
            .height = 2,
        };
        add_rect(screen, &approach, true);
    }
}

static void render_scene_normal(lv_obj_t *screen,
                                const struct zmk_dual_display_animation_plan *plan,
                                const struct zmk_dual_display_theme_snapshot *theme) {
    add_rect(screen, &plan->bounds, false);
    render_starfield(screen, &plan->bounds, theme);
    render_layer_modifier(screen, &plan->bounds, theme->layer);

    if (theme->variant == ZMK_DUAL_DISPLAY_SCENE_VARIANT_SECONDARY) {
        render_secondary_theme(screen, plan, theme);
    } else {
        render_primary_theme(screen, plan, theme);
    }
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

static void render_theme_phase_marker(lv_obj_t *screen,
                                      const struct zmk_dual_display_animation_plan *plan,
                                      const struct zmk_dual_display_theme_snapshot *theme) {
    const uint8_t intensity = phase_intensity(theme->phase);
    if (intensity == 0) {
        return;
    }

    for (uint8_t i = 0; i < intensity; i++) {
        const struct zmk_dual_display_rect marker = {
            .x = plan->bounds.x + 3 + i * 4,
            .y = plan->bounds.y + 4,
            .width = 2,
            .height = 2,
        };
        add_rect(screen, &marker, true);
    }
}

static struct zmk_dual_display_render_result render_animation_region(
    lv_obj_t *screen, const struct zmk_dual_display_screen_plan *screen_plan) {
    struct zmk_dual_display_theme_context *context = theme_context_for_side(screen_plan->side);
    zmk_dual_display_theme_context_observe_plan(context, screen_plan);
    const struct zmk_dual_display_theme_snapshot theme =
        zmk_dual_display_theme_context_snapshot(context);
    const struct zmk_dual_display_animation_plan *plan = &screen_plan->animation;

    switch (theme.scene) {
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
        render_scene_normal(screen, plan, &theme);
        break;
    }

    if (plan->charging) {
        render_charging_overlay(screen, plan);
    }

    render_theme_phase_marker(screen, plan, &theme);
    log_scene_change_once(plan);

    return (struct zmk_dual_display_render_result){
        .wants_next_frame = theme.wants_next_frame,
        .next_delay_ms = theme.next_delay_ms,
        .theme = theme,
    };
}

struct zmk_dual_display_render_result zmk_dual_display_lvgl_render_screen_plan(
    lv_obj_t *screen, const struct zmk_dual_display_screen_plan *plan) {
    if (screen == NULL || plan == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("mock skipped render for missing input: screen=%p plan=%p",
                                 (void *)screen, (const void *)plan);
        return (struct zmk_dual_display_render_result){0};
    }

    lv_obj_t *canvas = create_canvas(screen);
    if (canvas == NULL) {
        return (struct zmk_dual_display_render_result){0};
    }

    render_status_bar(canvas, &plan->status_bar);
    return render_animation_region(canvas, plan);
}
