SHELL := /usr/bin/env bash
VERIFY_IMAGE ?= zmk-config-verify

.PHONY: help verify verify-docker context checklist

help:
	@printf '%s\n' \
		'Available targets:' \
		'  make verify         Run cheap repo validation checks.' \
		'  make verify-docker  Run cheap validation checks in Docker.' \
		'  make context        Show the repo map used by agents.' \
		'  make checklist      Show the default change checklist.'

verify:
	@bash ./scripts/agentic/verify.sh

verify-docker:
	@docker build -f scripts/agentic/verify.Dockerfile -t $(VERIFY_IMAGE) .
	@docker run --rm -v "$(CURDIR):/workspace:ro" -w /workspace $(VERIFY_IMAGE)

context:
	@sed -n '1,220p' .agentic/context/repo-map.md

checklist:
	@sed -n '1,220p' .agentic/checklists/change.md
