/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#ifdef __ZEPHYR__
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk_dual_display, CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE_LOG_LEVEL);

#define ZMK_DUAL_DISPLAY_LOG_DBG(...) LOG_DBG(__VA_ARGS__)
#define ZMK_DUAL_DISPLAY_LOG_INF(...) LOG_INF(__VA_ARGS__)
#define ZMK_DUAL_DISPLAY_LOG_WRN(...) LOG_WRN(__VA_ARGS__)
#define ZMK_DUAL_DISPLAY_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#else
#define ZMK_DUAL_DISPLAY_LOG_DBG(...) do { } while (0)
#define ZMK_DUAL_DISPLAY_LOG_INF(...) do { } while (0)
#define ZMK_DUAL_DISPLAY_LOG_WRN(...) do { } while (0)
#define ZMK_DUAL_DISPLAY_LOG_ERR(...) do { } while (0)
#endif
