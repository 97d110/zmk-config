# Display Simulator

Increment 4 adds a small Ubuntu console simulator for the dual-display scene
engine. It compiles the durable `display/core/` C sources directly and renders
both screen plans as a compact ASCII preview.

Run it from the repo root:

```bash
make sim
```

Or from this directory:

```bash
make run
```

Run the browser simulator locally:

```bash
make sim-web
```

Run the browser simulator in Docker:

```bash
make sim-web-docker
```

Or from the Dockerfile directory:

```bash
make -C sim/web docker
```

The simulator accepts interactive commands on stdin. Useful commands:

- `show`
- `battery left 8`
- `battery right 80 charging`
- `activity left typing`
- `activity left idle`
- `sleep right on`
- `split left disconnected`
- `transport left usb`
- `layer right 2`
- `quit`

The preview is intentionally temporary and text-only. It must stay separate
from `display/core/`; the durable contract is the shared state and plan model,
not the ASCII renderer.

The web simulator is also an adapter around the compiled C simulator. It does
not duplicate the planner; each browser render is replayed through
`sim/build/dual_display_sim --batch`.
