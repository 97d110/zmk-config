/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <display/render/recipe/dual_display_compositor.h>

#include <stddef.h>
#include <string.h>

#include <display/log.h>

static bool sprite_bit(const uint8_t *bits, uint8_t width, int x, int y) {
    const int stride = (width + 7) / 8;
    return (bits[y * stride + (x >> 3)] >> (7 - (x & 7))) & 1u;
}

static void set_region_bit(struct zmk_dual_display_region_buffer *buffer, int x, int y, bool on) {
    if (x < 0 || x >= ZMK_DUAL_DISPLAY_REGION_WIDTH || y < 0 ||
        y >= ZMK_DUAL_DISPLAY_REGION_HEIGHT) {
        return;
    }

    uint8_t *byte = &buffer->bits[y * ZMK_DUAL_DISPLAY_REGION_STRIDE + (x >> 3)];
    const uint8_t bit = (uint8_t)(1u << (7 - (x & 7)));
    if (on) {
        *byte |= bit;
    } else {
        *byte &= (uint8_t)~bit;
    }
}

bool zmk_dual_display_region_get(const struct zmk_dual_display_region_buffer *buffer, int x, int y) {
    if (buffer == NULL || x < 0 || x >= ZMK_DUAL_DISPLAY_REGION_WIDTH || y < 0 ||
        y >= ZMK_DUAL_DISPLAY_REGION_HEIGHT) {
        return false;
    }

    return (buffer->bits[y * ZMK_DUAL_DISPLAY_REGION_STRIDE + (x >> 3)] >> (7 - (x & 7))) & 1u;
}

/*
 * Blit a source bitmap at (dx, dy), clipping to the region. `mask` may be NULL.
 * All source coordinates are clipped, so callers need not pre-clip clipped
 * sprites or negative origins.
 */
static void blit(struct zmk_dual_display_region_buffer *buffer, const uint8_t *pixels,
                 const uint8_t *mask, uint8_t width, uint8_t height, int dx, int dy,
                 enum zmk_dual_display_recipe_blend blend) {
    if (pixels == NULL) {
        return;
    }

    for (int sy = 0; sy < height; sy++) {
        for (int sx = 0; sx < width; sx++) {
            const int rx = dx + sx;
            const int ry = dy + sy;
            if (rx < 0 || rx >= ZMK_DUAL_DISPLAY_REGION_WIDTH || ry < 0 ||
                ry >= ZMK_DUAL_DISPLAY_REGION_HEIGHT) {
                continue;
            }

            const bool pixel = sprite_bit(pixels, width, sx, sy);
            switch (blend) {
            case ZMK_DUAL_DISPLAY_BLEND_OR_WHITE:
                if (pixel) {
                    set_region_bit(buffer, rx, ry, true);
                }
                break;
            case ZMK_DUAL_DISPLAY_BLEND_COPY_WITH_MASK: {
                const bool selected = (mask != NULL) ? sprite_bit(mask, width, sx, sy) : pixel;
                if (selected) {
                    set_region_bit(buffer, rx, ry, pixel);
                }
                break;
            }
            case ZMK_DUAL_DISPLAY_BLEND_CLEAR_BLACK:
                if (pixel) {
                    set_region_bit(buffer, rx, ry, false);
                }
                break;
            case ZMK_DUAL_DISPLAY_BLEND_INVERT_REGION:
            default:
                /* reserved for later glitch/error effects */
                break;
            }
        }
    }
}

static void render_command(const struct zmk_dual_display_recipe_command *command,
                           const struct zmk_dual_display_asset_source *assets,
                           struct zmk_dual_display_region_buffer *out) {
    struct zmk_dual_display_sprite sprite;

    switch (command->kind) {
    case ZMK_DUAL_DISPLAY_RECIPE_CLEAR_REGION:
        memset(out->bits, command->blend == ZMK_DUAL_DISPLAY_BLEND_OR_WHITE ? 0xFF : 0x00,
               sizeof(out->bits));
        break;
    case ZMK_DUAL_DISPLAY_RECIPE_DRAW_POINTS: {
        struct zmk_dual_display_point_set set;
        if (!assets->resolve_point_set(assets->ctx, command->point_set, &set)) {
            break;
        }
        for (uint16_t i = 0; i < set.count; i++) {
            if (!assets->resolve_sprite(assets->ctx, set.points[i].asset, 0, &sprite)) {
                continue;
            }
            blit(out, sprite.pixels, sprite.mask, sprite.width, sprite.height,
                 command->x + set.points[i].x, command->y + set.points[i].y, command->blend);
        }
        break;
    }
    case ZMK_DUAL_DISPLAY_RECIPE_DRAW_SPRITE:
    case ZMK_DUAL_DISPLAY_RECIPE_DRAW_CLIPPED_SPRITE:
        if (assets->resolve_sprite(assets->ctx, command->asset, command->frame, &sprite)) {
            blit(out, sprite.pixels, sprite.mask, sprite.width, sprite.height, command->x,
                 command->y, command->blend);
        }
        break;
    case ZMK_DUAL_DISPLAY_RECIPE_DRAW_SPRITE_MASKED:
        if (assets->resolve_sprite(assets->ctx, command->asset, command->frame, &sprite)) {
            blit(out, sprite.pixels, sprite.mask, sprite.width, sprite.height, command->x,
                 command->y, ZMK_DUAL_DISPLAY_BLEND_COPY_WITH_MASK);
        }
        break;
    case ZMK_DUAL_DISPLAY_RECIPE_APPLY_CLEARANCE_MASK:
        if (assets->resolve_sprite(assets->ctx, command->asset, command->frame, &sprite)) {
            const uint8_t *clearance = sprite.clearance != NULL ? sprite.clearance : sprite.pixels;
            blit(out, clearance, NULL, sprite.width, sprite.height, command->x, command->y,
                 ZMK_DUAL_DISPLAY_BLEND_CLEAR_BLACK);
        }
        break;
    default:
        break;
    }
}

void zmk_dual_display_compositor_render(const struct zmk_dual_display_recipe *recipe,
                                        const struct zmk_dual_display_asset_source *assets,
                                        struct zmk_dual_display_region_buffer *out) {
    if (out == NULL) {
        return;
    }

    memset(out->bits, 0x00, sizeof(out->bits));

    if (recipe == NULL || assets == NULL || assets->resolve_sprite == NULL ||
        assets->resolve_point_set == NULL) {
        ZMK_DUAL_DISPLAY_LOG_WRN("compositor skipped: recipe=%p assets=%p", (const void *)recipe,
                                 (const void *)assets);
        return;
    }

    for (uint8_t i = 0; i < recipe->command_count; i++) {
        render_command(&recipe->commands[i], assets, out);
    }
}
