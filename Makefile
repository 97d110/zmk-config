SHELL := /usr/bin/env bash

.PHONY: help verify context checklist

help:
	@printf '%s\n' \
		'Available targets:' \
		'  make verify     Run cheap repo validation checks.' \
		'  make context    Show the repo map used by agents.' \
		'  make checklist  Show the default change checklist.'

verify:
	@bash ./scripts/agentic/verify.sh

context:
	@sed -n '1,220p' .agentic/context/repo-map.md

checklist:
	@sed -n '1,220p' .agentic/checklists/change.md
