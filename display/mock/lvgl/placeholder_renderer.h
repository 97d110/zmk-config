/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <lvgl.h>

#include <display/core/dual_display_plan.h>
#include <display/render/lvgl/screen_renderer.h>

/*
 * Temporary proof-of-concept renderer.
 *
 * This adapter intentionally owns the hard-coded placeholder geometry. The
 * permanent LVGL renderer should keep consuming the core screen plan, but this
 * file can be replaced or deleted when real assets and animation playback are
 * introduced.
 */
/*
 * This header intentionally exposes no mock-specific firmware API. The mock
 * renderer implements zmk_dual_display_lvgl_render_screen_plan() from the
 * durable LVGL renderer contract.
 */
