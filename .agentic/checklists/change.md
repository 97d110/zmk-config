# Default Change Checklist

1. Confirm the change is in the right layer: board, config, workflow, docs, or generated assets.
2. Run `make verify`.
3. If the build matrix changed, confirm `build.yaml`, `AGENTS.md`, and `.agentic/commands.md` still agree.
4. If the change affects flashing or debugging, update the relevant workflow notes in `.agentic/commands.md`.
5. Do not edit `.zmk/` as part of normal repo changes.
