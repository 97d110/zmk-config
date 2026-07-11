/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <display/render/recipe/dual_display_asset_source.h>

/*
 * TEMPORARY placeholder asset backend for the space theme, v1.
 *
 * Resolves the theme's opaque asset IDs to hand-authored 1-bit placeholder
 * sprites (simple discs, dots, lines, blobs) and the real far/mid star
 * coordinate tables from the v13 manifest. It exists so the compositor can
 * render the recipe before real converted art exists, and is replaced by the
 * generated registry (roadmap Inc 9) behind this same interface.
 */
struct zmk_dual_display_asset_source zmk_dual_display_space_v1_mock_asset_source(void);
