# Copyright (c) 2026 The ZMK Contributors
# SPDX-License-Identifier: MIT

IMAGE ?= zmk-config-sim-web
PORT ?= 8080
ROOT_DIR := ../..

.PHONY: help docker-build docker-run docker

help:
	@printf '%s\n' \
		'Available targets:' \
		'  make docker-build  Build the simulator web Docker image.' \
		'  make docker-run    Run the simulator web app in Docker.' \
		'  make docker        Build and run the simulator web app in Docker.' \
		'' \
		'Variables:' \
		'  IMAGE=zmk-config-sim-web' \
		'  PORT=8080'

docker-build:
	@docker build -f Dockerfile -t $(IMAGE) $(ROOT_DIR)

docker-run:
	@docker run --rm -p $(PORT):8080 $(IMAGE)

docker: docker-build docker-run
