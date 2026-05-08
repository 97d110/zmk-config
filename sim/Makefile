# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic

ROOT_DIR := ..
BUILD_DIR := build
SIM_BIN := $(BUILD_DIR)/dual_display_sim

CORE_SRCS := \
	$(ROOT_DIR)/display/core/dual_display_state.c \
	$(ROOT_DIR)/display/core/dual_display_plan.c \
	$(ROOT_DIR)/display/render/theme/dual_display_theme.c

.PHONY: all run clean

all: $(SIM_BIN)

run: $(SIM_BIN)
	@$(SIM_BIN)

$(SIM_BIN): dual_display_sim.c $(CORE_SRCS) \
		$(ROOT_DIR)/display/core/dual_display_state.h \
		$(ROOT_DIR)/display/core/dual_display_plan.h \
		$(ROOT_DIR)/display/render/theme/dual_display_theme.h \
		$(ROOT_DIR)/display/log.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(ROOT_DIR) dual_display_sim.c $(CORE_SRCS) -o $@

clean:
	@rm -rf $(BUILD_DIR)
