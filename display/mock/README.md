# Display Mock Code

This subtree is temporary by design.

It contains proof-of-concept rendering code and placeholder geometry used to
validate the display contract before real scene assets, animation playback, and
state-driven rendering exist. Code here may depend on LVGL primitives and simple
hard-coded shapes because it is not the engine contract.

The placeholder must follow the portrait display contract: the top and bottom
edges are the short edges, and the left and right edges are the long edges.

## What Belongs Here

- Placeholder rendering.
- Mock icon shapes.
- Temporary scene geometry.
- Throwaway asset names used only by the placeholder renderer.
- Visual scaffolding used to prove layout, side selection, and logging.

## What Does Not Belong Here

- Shared state types.
- Scene planning policy.
- Layer, battery, transport, or split-link mapping logic.
- ZMK event adapters.
- Final art or durable asset registry code.

## Replacement Rule

Future increments should be able to delete this directory once a real renderer
and asset registry exist. If a piece of logic must survive that deletion, move it
to `display/core/`, `display/render/lvgl/`, `display/firmware/`, or
`display/assets/` first and document why it is no longer mock code.
