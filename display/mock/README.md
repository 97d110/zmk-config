# Display Mock Code

This subtree is temporary by design.

It contains proof-of-concept rendering code and placeholder geometry used to
validate the display contract before real scene assets, animation playback, and
state-driven rendering exist. Code here may depend on LVGL primitives and simple
hard-coded shapes because it is not the engine contract. The mock renderer may
consume the shared animation context, but it must keep throwaway shape composition
inside this subtree.

The placeholder must follow the portrait display contract: the top and bottom
edges are the short edges, and the left and right edges are the long edges.

## What Belongs Here

- Placeholder rendering.
- Mock icon shapes.
- Temporary scene geometry.
- Throwaway asset names used only by the placeholder renderer.
- Visual scaffolding used to prove layout, side selection, and logging.
- No-asset drawings that make theme phase, decay, layer, energy, and charging
  state visible during development.

## What Does Not Belong Here

- Shared state types.
- Scene planning policy.
- Layer, battery, transport, or split-link mapping logic.
- Animation state-machine policy that must also run in the simulator.
- ZMK event adapters.
- Final art or durable asset registry code.

## Replacement Rule

Future increments should be able to delete this directory once a real renderer
and asset registry exist. If a piece of logic must survive that deletion, move it
to `display/core/`, `display/render/lvgl/`, `display/render/recipe/`,
`display/firmware/`, or the theme under `themes/<name>/<version>/` first and
document why it is no longer mock code.
