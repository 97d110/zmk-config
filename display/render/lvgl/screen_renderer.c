/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * Durable LVGL renderer. Draws the portrait status bar from the Display Plan and
 * renders the animation region by compositing the theme's render recipe into a
 * 1-bit region buffer, then blitting it onto the nice!view canvas. The active
 * theme (space/v1) and its asset backend are selected here; a future
 * theme-selection layer can abstract that choice.
 */

#include <display/render/lvgl/screen_renderer.h>

#include <display/log.h>
#include <display/render/animation/dual_display_animation.h>
#include <display/render/lvgl/viewport.h>
#include <display/render/recipe/dual_display_compositor.h>
#include <display/render/recipe/dual_display_recipe.h>
#include <themes/space/v1/mock/mock_assets.h>
#include <themes/space/v1/scene_recipe.h>

static lv_color_t canvas_buf[ZMK_DUAL_DISPLAY_LONG_EDGE * ZMK_DUAL_DISPLAY_SHORT_EDGE];
static struct zmk_dual_display_animation_context animation_contexts[2];
static struct zmk_dual_display_region_buffer region_buffer;

static lv_obj_t *create_canvas(lv_obj_t *screen) {
    lv_obj_t *canvas = lv_canvas_create(screen);

    if (canvas == NULL) {
        ZMK_DUAL_DISPLAY_LOG_ERR("failed to create display canvas");
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
        ZMK_DUAL_DISPLAY_LOG_WRN("skipped rectangle for missing input: canvas=%p bounds=%p",
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

static struct zmk_dual_display_animation_context *animation_context_for_side(
    enum zmk_dual_display_side side) {
    return &animation_contexts[side == ZMK_DUAL_DISPLAY_SIDE_RIGHT ? 1 : 0];
}

/*
 * Blit the composited 1-bit region buffer onto the canvas. The animation region
 * is drawn black (space background) with white lit pixels. Region-relative pixel
 * (rx, ry) maps to the panel's landscape framebuffer via the same portrait
 * mapping used by add_rect / the viewport.
 */
static void blit_region(lv_obj_t *canvas, const struct zmk_dual_display_region_buffer *region) {
    const lv_color_t on = lv_color_white();
    const lv_color_t off = lv_color_black();

    for (uint8_t ry = 0; ry < ZMK_DUAL_DISPLAY_REGION_HEIGHT; ry++) {
        for (uint8_t rx = 0; rx < ZMK_DUAL_DISPLAY_REGION_WIDTH; rx++) {
            const lv_coord_t px =
                (lv_coord_t)(ZMK_DUAL_DISPLAY_HEIGHT - 1 - (ZMK_DUAL_DISPLAY_ANIMATION_Y + ry));
            const lv_coord_t py = (lv_coord_t)rx;
            lv_canvas_set_px_color(canvas, px, py,
                                   zmk_dual_display_region_get(region, rx, ry) ? on : off);
        }
    }
}

static void log_recipe_render_once(const struct zmk_dual_display_screen_plan *screen_plan,
                                   const struct zmk_dual_display_animation_snapshot *snapshot,
                                   const struct zmk_dual_display_recipe *recipe) {
    static int last_scene[2] = {-1, -1};
    static int last_phase[2] = {-1, -1};
    const uint8_t idx = screen_plan->side == ZMK_DUAL_DISPLAY_SIDE_RIGHT ? 1 : 0;

    if ((int)snapshot->scene == last_scene[idx] && (int)snapshot->phase == last_phase[idx]) {
        return;
    }
    last_scene[idx] = (int)snapshot->scene;
    last_phase[idx] = (int)snapshot->phase;

    zmk_dual_display_asset_id_t actor = ZMK_DUAL_DISPLAY_ASSET_NONE;
    for (uint8_t i = 0; i < recipe->command_count; i++) {
        if (recipe->commands[i].kind == ZMK_DUAL_DISPLAY_RECIPE_DRAW_SPRITE_MASKED) {
            actor = recipe->commands[i].asset;
            break;
        }
    }

    ZMK_DUAL_DISPLAY_LOG_DBG("recipe rendered: side=%s scene=%s phase=%s commands=%u actor_asset=%u",
                             zmk_dual_display_side_name(snapshot->side),
                             zmk_dual_display_animation_scene_name(snapshot->scene),
                             zmk_dual_display_animation_phase_name(snapshot->phase),
                             (unsigned int)recipe->command_count, (unsigned int)actor);
}

static struct zmk_dual_display_render_result render_animation_region(
    lv_obj_t *canvas, const struct zmk_dual_display_screen_plan *screen_plan) {
    struct zmk_dual_display_animation_context *context =
        animation_context_for_side(screen_plan->side);
    zmk_dual_display_animation_context_observe_plan(context, screen_plan);
    const struct zmk_dual_display_animation_snapshot snapshot =
        zmk_dual_display_animation_context_snapshot(context);

    struct zmk_dual_display_recipe recipe;
    zmk_dual_display_space_v1_build_recipe(&snapshot, &screen_plan->animation, &recipe);

    const struct zmk_dual_display_asset_source assets =
        zmk_dual_display_space_v1_mock_asset_source();
    zmk_dual_display_compositor_render(&recipe, &assets, &region_buffer);
    blit_region(canvas, &region_buffer);

    log_recipe_render_once(screen_plan, &snapshot, &recipe);

    return (struct zmk_dual_display_render_result){
        .wants_next_frame = snapshot.wants_next_frame,
        .next_delay_ms = snapshot.next_delay_ms,
        .theme = snapshot,
    };
}

struct zmk_dual_display_render_result zmk_dual_display_lvgl_render_screen_plan(
    lv_obj_t *screen, const struct zmk_dual_display_screen_plan *plan) {
    if (screen == NULL || plan == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("renderer skipped for missing input: screen=%p plan=%p",
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
