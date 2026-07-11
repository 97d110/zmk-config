# Code Organization Convention

This convention applies whenever planning or writing code in this repository.

## Required Distinction

Always decide whether new logic, assets, scripts, generated outputs, fixtures,
or scaffolding are durable product code or temporary/mock code before choosing
their location and names.

Durable code is intended to remain part of the product or repo workflow. It
should use generic, behavior-oriented names and live in the subsystem that owns
the real responsibility.

Temporary/mock code exists to prove a concept, test a path, stand in for missing
assets, or support a short-lived development workflow. It must be isolated so it
can be replaced or deleted without rewriting durable code.

## Naming Rules

- Durable names must describe generic behavior or contracts, not current test
  scaffolding: examples include `scene_variant`, `status_slot`,
  `animation_plan`, `state_bucket`, and `screen_plan`.
- Temporary names must make their status obvious: use words such as `mock`,
  `placeholder`, `fixture`, `sample`, `poc`, or `temporary`.
- Do not put final art style names, donor repo names, fixture names, or
  placeholder asset names into durable public APIs.
- If a temporary value needs to flow through durable code, represent it as a
  generic durable concept and keep the mock-specific interpretation behind the
  temporary boundary.

## Planning Rule

Every implementation plan should explicitly state:

- which parts are durable,
- which parts are temporary/mock,
- how temporary parts can be replaced or deleted later,
- whether any temporary logic is leaking into durable APIs.

## Implementation Rule

When writing code:

- keep temporary files in a clearly named boundary such as `mock/`, `fixtures/`,
  `samples/`, `.tmp/`, or a subsystem-specific equivalent,
- document temporary files with a short replacement/deletion note,
- keep durable files free of mock-only naming and throwaway asset concepts,
- update this convention if a future subsystem needs a different boundary.

## Current Display-Engine Application

For the local dual nice!view display engine, generic reusable modules live under
`display/` and theme-specific content lives under root `themes/`:

- durable planning belongs under `display/core/`,
- durable LVGL adapter code, renderer contracts, and viewport mapping belong
  under `display/render/lvgl/`,
- the durable generic animation controller (typing phase, decay, frame clock,
  animation snapshot) belongs under `display/render/animation/`,
- the durable generic composition instruction set and 1-bit compositor belong
  under `display/render/recipe/`; they reference assets by opaque integer ID and
  stay theme-independent,
- theme-specific content — the scene-recipe planner, asset-ID vocabulary, timing
  profile, source assets, and any mock asset backend — belongs under
  `themes/<name>/<version>/` (e.g. `themes/space/v1/`), never under `display/`,
- the only temporary code is a theme's mock asset backend
  (`themes/<name>/<version>/mock/`), a placeholder until the real converted asset
  registry exists; it stays deletable behind the recipe asset-source interface.
- all durable and temporary display layout code must preserve the physical
  portrait contract: short top/bottom edges, long left/right edges.
