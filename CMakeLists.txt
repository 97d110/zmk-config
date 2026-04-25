# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

if(CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE)
  zephyr_library()
  zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  zephyr_library_sources(display/core/dual_display_plan.c)
  zephyr_library_sources(display/render/lvgl/dual_display_status_screen.c)
endif()
