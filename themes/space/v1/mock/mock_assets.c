/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 * TEMPORARY placeholder asset backend. Replace with the generated 1-bit registry
 * (roadmap Inc 9) behind the same asset-source interface.
 */

#include <themes/space/v1/mock/mock_assets.h>

#include <stddef.h>
#include <string.h>

#include <themes/space/v1/assets.h>

/* Largest sprite is the galaxy sheet (129x98). Scratch buffers hold the sprite
 * currently being resolved; the compositor consumes each resolved sprite before
 * requesting the next. */
#define MOCK_MAX_STRIDE ((129 + 7) / 8)
#define MOCK_MAX_BYTES (MOCK_MAX_STRIDE * 98)

static uint8_t s_pixels[MOCK_MAX_BYTES];
static uint8_t s_mask[MOCK_MAX_BYTES];
static uint8_t s_clearance[MOCK_MAX_BYTES];

static void px_set(uint8_t *buf, int stride, int x, int y) {
    if (x < 0 || y < 0) {
        return;
    }
    buf[y * stride + (x >> 3)] |= (uint8_t)(1u << (7 - (x & 7)));
}

static void px_rect(uint8_t *buf, int stride, int x0, int y0, int w, int h) {
    for (int y = y0; y < y0 + h; y++) {
        for (int x = x0; x < x0 + w; x++) {
            px_set(buf, stride, x, y);
        }
    }
}

static void px_disc(uint8_t *buf, int stride, int cx, int cy, int r) {
    for (int y = cy - r; y <= cy + r; y++) {
        for (int x = cx - r; x <= cx + r; x++) {
            const int dx = x - cx;
            const int dy = y - cy;
            if (dx * dx + dy * dy <= r * r) {
                px_set(buf, stride, x, y);
            }
        }
    }
}

static bool solid_sprite(uint8_t w, uint8_t h, uint8_t frame_count,
                         struct zmk_dual_display_sprite *out) {
    const int stride = (w + 7) / 8;
    memset(s_pixels, 0, (size_t)(stride * h));
    px_rect(s_pixels, stride, 0, 0, w, h);
    *out = (struct zmk_dual_display_sprite){s_pixels, NULL, NULL, w, h, frame_count};
    return true;
}

static bool mock_resolve_sprite(void *ctx, zmk_dual_display_asset_id_t asset, uint8_t frame,
                                struct zmk_dual_display_sprite *out) {
    (void)ctx;

    switch (asset) {
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_ASTEROID: {
        const int stride = (32 + 7) / 8;
        memset(s_pixels, 0, (size_t)(stride * 32));
        memset(s_mask, 0, (size_t)(stride * 32));
        memset(s_clearance, 0, (size_t)(stride * 32));
        px_disc(s_pixels, stride, 16, 16, 13);
        px_disc(s_mask, stride, 16, 16, 13);
        px_disc(s_clearance, stride, 16, 16, 15);
        *out = (struct zmk_dual_display_sprite){s_pixels, s_mask, s_clearance, 32, 32, 16};
        return true;
    }
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1: {
        s_pixels[0] = 0x80;
        *out = (struct zmk_dual_display_sprite){s_pixels, NULL, NULL, 1, 1, 1};
        return true;
    }
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_2:
        return solid_sprite(2, 2, 1, out);
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_PLUS_SMALL: {
        memset(s_pixels, 0, 3);
        px_set(s_pixels, 1, 1, 0);
        px_set(s_pixels, 1, 0, 1);
        px_set(s_pixels, 1, 1, 1);
        px_set(s_pixels, 1, 2, 1);
        px_set(s_pixels, 1, 1, 2);
        *out = (struct zmk_dual_display_sprite){s_pixels, NULL, NULL, 3, 3, 1};
        return true;
    }
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_TWINKLE_LARGE_GLOW: {
        const int stride = (13 + 7) / 8;
        memset(s_pixels, 0, (size_t)(stride * 13));
        px_disc(s_pixels, stride, 6, 6, 4);
        *out = (struct zmk_dual_display_sprite){s_pixels, NULL, NULL, 13, 13, 64};
        return true;
    }
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_00:
        return solid_sprite(11, 12, 1, out);
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_01:
        return solid_sprite(9, 10, 1, out);
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_02:
        return solid_sprite(8, 9, 1, out);
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_03:
        return solid_sprite(9, 10, 1, out);
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_04:
        return solid_sprite(8, 9, 1, out);
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_SPEED_STREAK_05:
        return solid_sprite(6, 7, 1, out);
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_GALAXY_CORE_ARMS: {
        const int stride = (129 + 7) / 8;
        memset(s_pixels, 0, (size_t)(stride * 98));
        memset(s_mask, 0, (size_t)(stride * 98));
        px_disc(s_pixels, stride, 55, 48, 22);
        px_disc(s_mask, stride, 55, 48, 22);
        *out = (struct zmk_dual_display_sprite){s_pixels, s_mask, NULL, 129, 98, 1};
        return true;
    }
    case ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_GALAXY_EDGE_STARS: {
        static const struct {
            int16_t x;
            int16_t y;
        } edge[] = {{20, 10}, {110, 15}, {100, 88}, {18, 90}, {60, 4}, {125, 50}, {5, 55}, {64, 95}};
        const int stride = (129 + 7) / 8;
        memset(s_pixels, 0, (size_t)(stride * 98));
        for (size_t i = 0; i < sizeof(edge) / sizeof(edge[0]); i++) {
            if (((i + frame) & 1u) == 0) { /* sparse blink over the 64-frame loop */
                px_set(s_pixels, stride, edge[i].x, edge[i].y);
            }
        }
        *out = (struct zmk_dual_display_sprite){s_pixels, NULL, NULL, 129, 98, 64};
        return true;
    }
    default:
        return false;
    }
}

/* Real far/mid star coordinate tables from the v13 manifest (region-relative). */
static const struct zmk_dual_display_point FAR_STARS[] = {
    {45, 104, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {46, 55, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {22, 40, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {18, 143, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {19, 101, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {19, 56, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {37, 30, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {50, 132, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {34, 133, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {27, 49, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {39, 14, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {39, 15, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {67, 94, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {3, 108, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {26, 51, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {11, 6, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {8, 72, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {53, 38, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {13, 140, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {57, 60, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {42, 117, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {59, 45, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {17, 13, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {2, 120, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {39, 93, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {39, 137, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {34, 78, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {18, 102, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {59, 122, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {44, 106, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {15, 81, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {64, 96, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {64, 53, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {48, 75, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {3, 94, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {55, 49, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {44, 1, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {34, 117, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {0, 126, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {48, 120, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {63, 144, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {47, 23, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {3, 14, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {5, 109, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {19, 88, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {9, 12, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {67, 80, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {60, 3, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {46, 127, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {51, 100, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {45, 73, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {13, 138, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {64, 126, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {7, 143, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {12, 78, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {31, 143, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {52, 9, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {13, 11, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
};

static const struct zmk_dual_display_point MID_STARS[] = {
    {53, 111, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {2, 60, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {30, 48, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_2},
    {53, 70, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {30, 10, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {22, 116, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {20, 90, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_2},
    {15, 41, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_PLUS_SMALL},
    {45, 24, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {55, 47, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {33, 26, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_PLUS_SMALL},
    {19, 127, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_2},
    {54, 45, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
    {50, 139, ZMK_DUAL_DISPLAY_SPACE_V1_ASSET_STAR_DOT_1},
};

static bool mock_resolve_point_set(void *ctx, zmk_dual_display_point_set_id_t id,
                                   struct zmk_dual_display_point_set *out) {
    (void)ctx;

    switch (id) {
    case ZMK_DUAL_DISPLAY_SPACE_V1_POINTS_FAR_STARS:
        *out = (struct zmk_dual_display_point_set){FAR_STARS,
                                                   sizeof(FAR_STARS) / sizeof(FAR_STARS[0])};
        return true;
    case ZMK_DUAL_DISPLAY_SPACE_V1_POINTS_MID_STARS:
        *out = (struct zmk_dual_display_point_set){MID_STARS,
                                                   sizeof(MID_STARS) / sizeof(MID_STARS[0])};
        return true;
    default:
        return false;
    }
}

struct zmk_dual_display_asset_source zmk_dual_display_space_v1_mock_asset_source(void) {
    return (struct zmk_dual_display_asset_source){
        .resolve_sprite = mock_resolve_sprite,
        .resolve_point_set = mock_resolve_point_set,
        .ctx = NULL,
    };
}
