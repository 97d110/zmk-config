/*
 * Copyright (c) 2026 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT zmk_behavior_dual_display_layer_sync

#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <display/firmware/dual_display_state_adapter.h>

static int on_keymap_binding_pressed(struct zmk_behavior_binding *binding,
                                     struct zmk_behavior_binding_event event) {
    ARG_UNUSED(event);

    return zmk_dual_display_firmware_apply_layer_index((uint8_t)binding->param1,
                                                       "split-layer-sync");
}

static int on_keymap_binding_released(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

    return 0;
}

static const struct behavior_driver_api behavior_dual_display_layer_sync_driver_api = {
    .binding_pressed = on_keymap_binding_pressed,
    .binding_released = on_keymap_binding_released,
};

#define DDL_SYNC_INST(n)                                                                           \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL, POST_KERNEL,                                \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                                    \
                            &behavior_dual_display_layer_sync_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DDL_SYNC_INST)
