# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

if(CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)

  set(ZMK_DUAL_DISPLAY_GENERATED_INCLUDE_DIR
      ${CMAKE_CURRENT_BINARY_DIR}/generated)
  set(ZMK_DUAL_DISPLAY_ANIMATION_TIMING_HEADER
      ${ZMK_DUAL_DISPLAY_GENERATED_INCLUDE_DIR}/display/render/animation/dual_display_animation_timing.h)

  execute_process(
    COMMAND
      ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/scripts/agentic/generate_animation_timing.py
      --input ${CMAKE_CURRENT_SOURCE_DIR}/themes/space/v1/timing_profile.json
      --output ${ZMK_DUAL_DISPLAY_ANIMATION_TIMING_HEADER}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY)

  zephyr_library()
  zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  zephyr_library_include_directories(${ZMK_DUAL_DISPLAY_GENERATED_INCLUDE_DIR})
  target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_include_directories(app PRIVATE ${ZMK_DUAL_DISPLAY_GENERATED_INCLUDE_DIR})
  zephyr_library_sources(display/core/dual_display_state.c)
  zephyr_library_sources(display/core/dual_display_plan.c)
  zephyr_library_sources(display/render/animation/dual_display_animation.c)
  zephyr_library_sources(display/render/recipe/dual_display_recipe.c)
  zephyr_library_sources(display/render/recipe/dual_display_compositor.c)
  # Active theme (space/v1) sources.
  zephyr_library_sources(themes/space/v1/scene_recipe.c)
  zephyr_library_sources(themes/space/v1/mock/mock_assets.c)
  target_sources(app PRIVATE display/firmware/dual_display_state_adapter.c)
  target_sources(app PRIVATE display/firmware/dual_display_layer_sync_behavior.c)
  zephyr_library_sources(display/render/lvgl/viewport.c)
  zephyr_library_sources(display/render/lvgl/dual_display_status_screen.c)
  zephyr_library_sources(display/render/lvgl/screen_renderer.c)
endif()
