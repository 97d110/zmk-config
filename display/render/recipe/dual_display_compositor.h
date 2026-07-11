/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <display/render/recipe/dual_display_asset_source.h>
#include <display/render/recipe/dual_display_recipe.h>

/*
 * Generic 1-bit compositor. Executes a recipe into a region-relative 1-bit
 * framebuffer for the 68x146 animation region, resolving asset IDs through the
 * asset source. Deterministic and LVGL-free: firmware and the simulator run the
 * same compositor over the same recipe + assets for identical pixels.
 */

#define ZMK_DUAL_DISPLAY_REGION_WIDTH ZMK_DUAL_DISPLAY_SHORT_EDGE
#define ZMK_DUAL_DISPLAY_REGION_HEIGHT ZMK_DUAL_DISPLAY_ANIMATION_HEIGHT
#define ZMK_DUAL_DISPLAY_REGION_STRIDE ((ZMK_DUAL_DISPLAY_REGION_WIDTH + 7) / 8)
#define ZMK_DUAL_DISPLAY_REGION_BYTES (ZMK_DUAL_DISPLAY_REGION_STRIDE * ZMK_DUAL_DISPLAY_REGION_HEIGHT)

struct zmk_dual_display_region_buffer {
    uint8_t bits[ZMK_DUAL_DISPLAY_REGION_BYTES]; /* 1 = on/white, 0 = off/black */
};

/* Read a region pixel (region-relative). Out-of-bounds reads return false. */
bool zmk_dual_display_region_get(const struct zmk_dual_display_region_buffer *buffer, int x, int y);

/* Execute the recipe into the region buffer using the asset source. */
void zmk_dual_display_compositor_render(const struct zmk_dual_display_recipe *recipe,
                                        const struct zmk_dual_display_asset_source *assets,
                                        struct zmk_dual_display_region_buffer *out);
