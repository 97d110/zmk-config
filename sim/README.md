# Display Simulator

The simulator is a browser canvas app served from localhost. It renders the
dual nice!view displays directly in HTML/CSS/canvas and listens to USB CDC logs
from debug firmware through the local Python serial bridge. Keyboard events are
fed into a host C runner built from `display/core/` and `display/render/theme/`;
the canvas renders the C-derived snapshot.

Run it from the repo root:

```bash
make sim
```

The equivalent explicit target is:

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

Open the page at `http://localhost:8080`. The local serial bridge scans
`/dev/serial/by-id/*` and `/dev/ttyACM*`, so it can read whichever USB CDC
interface is actually emitting debug logs.

The keyboard is the controller: core key/layer events from the logs drive the
host C display/theme engine. Firmware display/theme logs remain visible for
comparison, but they do not control the canvas state. This simulator is not a
firmware build path and does not use local `west`.

The timing editor changes only the Theme State Timing Profile. It does not
create a second source of Core State or manual display behavior. Edits are sent
to the host C engine using the same profile shape as
`display/render/theme/timing_profile.json`; the export pane can be committed
back to that JSON after tuning.

Run the scripted timing checks with:

```bash
make sim-test
```

If the displays do not react, watch the `local serial` status and `serialParse`
counter in the left log pane. `lines` increasing with `snapshots` stuck at zero
usually means the host C engine did not start. Zero local serial ports usually
means the keyboard is not visible to the server process or the user running
`make sim` cannot read the tty devices.
