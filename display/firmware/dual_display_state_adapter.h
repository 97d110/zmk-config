/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

#include <display/core/dual_display_state.h>

void zmk_dual_display_firmware_init_state(enum zmk_dual_display_side side,
                                          struct zmk_dual_display_state *out_state);

void zmk_dual_display_firmware_schedule_animation_refresh(uint32_t delay_ms);

int zmk_dual_display_firmware_apply_layer_index(uint8_t layer_index, const char *reason);
