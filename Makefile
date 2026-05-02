SHELL := /usr/bin/env bash
VERIFY_IMAGE ?= zmk-config-verify
SIM_WEB_IMAGE ?= zmk-config-sim-web
SIM_WEB_PORT ?= 8080

.PHONY: help verify verify-docker context checklist sim sim-build sim-clean sim-web sim-web-docker-build sim-web-docker

help:
	@printf '%s\n' \
		'Available targets:' \
		'  make verify         Run cheap repo validation checks.' \
		'  make verify-docker  Run cheap validation checks in Docker.' \
		'  make sim            Run the desktop display simulator.' \
		'  make sim-web        Run the browser simulator locally.' \
		'  make sim-web-docker Run the browser simulator in Docker.' \
		'  make context        Show the repo map used by agents.' \
		'  make checklist      Show the default change checklist.'

verify:
	@bash ./scripts/agentic/verify.sh

verify-docker:
	@docker build -f scripts/agentic/verify.Dockerfile -t $(VERIFY_IMAGE) .
	@docker run --rm -v "$(CURDIR):/workspace:ro" -w /workspace $(VERIFY_IMAGE)

sim:
	@$(MAKE) -C sim run

sim-build:
	@$(MAKE) -C sim all

sim-clean:
	@$(MAKE) -C sim clean

sim-web:
	@python3 sim/web/app.py

sim-web-docker-build:
	@docker build -f sim/web/Dockerfile -t $(SIM_WEB_IMAGE) .

sim-web-docker: sim-web-docker-build
	@docker run --rm -p $(SIM_WEB_PORT):8080 $(SIM_WEB_IMAGE)

context:
	@sed -n '1,220p' .agentic/context/repo-map.md

checklist:
	@sed -n '1,220p' .agentic/checklists/change.md
