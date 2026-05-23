# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

if(CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)

  set(ZMK_DUAL_DISPLAY_GENERATED_INCLUDE_DIR
      ${CMAKE_CURRENT_BINARY_DIR}/generated)
  set(ZMK_DUAL_DISPLAY_THEME_TIMING_HEADER
      ${ZMK_DUAL_DISPLAY_GENERATED_INCLUDE_DIR}/display/render/theme/dual_display_theme_timing.h)

  execute_process(
    COMMAND
      ${Python3_EXECUTABLE}
      ${CMAKE_CURRENT_SOURCE_DIR}/scripts/agentic/generate_theme_timing.py
      --input ${CMAKE_CURRENT_SOURCE_DIR}/display/render/theme/timing_profile.json
      --output ${ZMK_DUAL_DISPLAY_THEME_TIMING_HEADER}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY)

  zephyr_library()
  zephyr_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
  zephyr_library_include_directories(${ZMK_DUAL_DISPLAY_GENERATED_INCLUDE_DIR})
  target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_include_directories(app PRIVATE ${ZMK_DUAL_DISPLAY_GENERATED_INCLUDE_DIR})
  zephyr_library_sources(display/core/dual_display_state.c)
  zephyr_library_sources(display/core/dual_display_plan.c)
  zephyr_library_sources(display/render/theme/dual_display_theme.c)
  target_sources(app PRIVATE display/firmware/dual_display_state_adapter.c)
  zephyr_library_sources(display/render/lvgl/viewport.c)
  zephyr_library_sources(display/render/lvgl/dual_display_status_screen.c)

  if(CONFIG_ZMK_DUAL_DISPLAY_SCENE_ENGINE_MOCK_RENDERER)
    zephyr_library_sources(display/mock/lvgl/placeholder_renderer.c)
  endif()
endif()
