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
#include <stdio.h>

#define ZMK_DUAL_DISPLAY_SIM_LOG(level, ...)                                                \
    do {                                                                                    \
        fprintf(stderr, "<%s> zmk_dual_display: %s: ", level, __func__);                    \
        fprintf(stderr, __VA_ARGS__);                                                       \
        fputc('\n', stderr);                                                                \
    } while (0)

#define ZMK_DUAL_DISPLAY_LOG_DBG(...) ZMK_DUAL_DISPLAY_SIM_LOG("dbg", __VA_ARGS__)
#define ZMK_DUAL_DISPLAY_LOG_INF(...) ZMK_DUAL_DISPLAY_SIM_LOG("inf", __VA_ARGS__)
#define ZMK_DUAL_DISPLAY_LOG_WRN(...) ZMK_DUAL_DISPLAY_SIM_LOG("wrn", __VA_ARGS__)
#define ZMK_DUAL_DISPLAY_LOG_ERR(...) ZMK_DUAL_DISPLAY_SIM_LOG("err", __VA_ARGS__)
#endif
