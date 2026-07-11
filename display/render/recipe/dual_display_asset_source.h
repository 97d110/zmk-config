/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <display/render/recipe/dual_display_recipe.h>

/*
 * Generic asset-backend interface consumed by the compositor.
 *
 * The compositor resolves opaque asset IDs and point-set IDs to 1-bit pixel data
 * through this interface. A theme implements it (mock stand-in first, generated
 * registry later) without the compositor learning anything theme-specific.
 *
 * 1-bit bitmap format: row-major, MSB-first, stride = (width + 7) / 8 bytes per
 * row, bit value 1 = "set" (on/white).
 */

struct zmk_dual_display_sprite {
    const uint8_t *pixels;    /* required */
    const uint8_t *mask;      /* NULL when the sprite has no separate mask */
    const uint8_t *clearance; /* NULL when the sprite has no clearance mask */
    uint8_t width;
    uint8_t height;
    uint8_t frame_count;
};

struct zmk_dual_display_point {
    int16_t x;
    int16_t y;
    zmk_dual_display_asset_id_t asset;
};

struct zmk_dual_display_point_set {
    const struct zmk_dual_display_point *points;
    uint16_t count;
};

struct zmk_dual_display_asset_source {
    /* Resolve an asset id + frame index to sprite pixel data. Returns false when
     * the id or frame is unknown. */
    bool (*resolve_sprite)(void *ctx, zmk_dual_display_asset_id_t asset, uint8_t frame,
                           struct zmk_dual_display_sprite *out);
    /* Resolve a point-set id to its coordinate list. Returns false when unknown. */
    bool (*resolve_point_set)(void *ctx, zmk_dual_display_point_set_id_t id,
                              struct zmk_dual_display_point_set *out);
    void *ctx;
};
