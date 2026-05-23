# Assembly Guide

Render on a 68x146 black canvas. For full nice!view output, paste the canvas at y=14 into a 68x160 frame.

Render order:

1. black canvas
2. far stars
3. galaxy core/arms
4. galaxy outer glow
5. mid stars
6. twinkle instances
7. speed streaks
8. asteroid clearance mask
9. asteroid sprite

Use `manifest.json` as the source of truth.

Main formulas:

```ts
const frameCount = 64;
const t = frame / frameCount;

// Asteroid
const s = Math.sin(Math.PI * t);
const asteroidX = 11 + (17 - 11) * (s ** 2) + 1.3 * s * Math.sin(4 * Math.PI * t);
const asteroidY = 20 + (46 - 20) * s + 0.9 * s * Math.sin(6 * Math.PI * t + 0.8);
const asteroidSprite = Math.floor(frame / 4) % 16;

// Galaxy
const gx = 48 + Math.round(-1 * Math.sin(2 * Math.PI * t));
const gy = 106 + Math.round(-1 * Math.sin(2 * Math.PI * t + 0.6));
const galaxyTopLeftX = gx - galaxy.origin_in_sprite[0];
const galaxyTopLeftY = gy - galaxy.origin_in_sprite[1];
```

Twinkle path uses `frame / (frameCount - 1)` so the visible travel starts and ends outside the frame.

For parallax layers, wrap positions with modulo canvas width/height.
