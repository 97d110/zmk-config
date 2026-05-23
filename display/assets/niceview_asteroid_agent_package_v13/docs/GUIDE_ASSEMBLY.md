# GUIDE: Assembly

## Canvas
- Animation region: `68 x 146`
- Full display: `68 x 160`
- When generating full display frames, paste the animation region at `y = 14`.

## Layer order
Render every frame in this order:

1. black canvas
2. far stars
3. galaxy core/arms
4. galaxy edge stars
5. mid stars
6. twinkle instances
7. speed streak instances
8. asteroid clearance mask
9. asteroid sprite

## Key change in this version
The galaxy layer has been split differently:

- `galaxy_core_arms` now includes:
  - core
  - arms
  - inner ring
  - non-sparse parts of the former outer ring

- `galaxy_edge_stars` now includes:
  - only sparse singular distant stars around the galaxy edge
  - these stars blink in and out over time

This means the animated galaxy edge layer is no longer a broad outer glow ring. It is specifically a sparse distant star field attached to the galaxy perimeter.

## Main formulas

### Global frame
```ts
const frameCount = 64;
const t = frameIndex / frameCount;
```

### Asteroid motion
```ts
const s = Math.sin(Math.PI * t);
const x = 11 + (17 - 11) * (s ** 2) + 1.3 * s * Math.sin(4 * Math.PI * t);
const y = 20 + (46 - 20) * s + 0.9 * s * Math.sin(6 * Math.PI * t + 0.8);
const asteroidFrame = Math.floor(frameIndex / 4) % 16;
```

### Galaxy placement
```ts
const gx = 48 + Math.round(-1 * Math.sin(2 * Math.PI * t));
const gy = 106 + Math.round(-1 * Math.sin(2 * Math.PI * t + 0.6));

const galaxyTopLeftX = gx - galaxy.origin_in_sprite[0];
const galaxyTopLeftY = gy - galaxy.origin_in_sprite[1];
```

Draw both galaxy assets at the same top-left position.

### Twinkle paths
Twinkles use:

```ts
const tt = frameIndex / (frameCount - 1);
```

This makes the path start and end outside the frame.

### Parallax
Use wrapped parallax movement:

```ts
far_dx  = round(-68  * t); far_dy  = round(-146 * t);
mid_dx  = round(-136 * t); mid_dy  = round(-292 * t);
fast_dx = round(-204 * t); fast_dy = round(-438 * t);
```

## Rebuilding the example GIF
1. Assemble the layers as described above.
2. Save 64 frames.
3. Encode them as a looping GIF at `70 ms` per frame.

Use:
- `example/output_native_68x146.gif`
- `output_frames/frame_###.png`

as your exact reference output.
