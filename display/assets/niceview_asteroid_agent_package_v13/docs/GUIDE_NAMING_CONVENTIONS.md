# GUIDE: Naming Conventions

All PNG assets are pure 1-bit black/white.

## Output frames
The assembled animation frames are stored as:

```txt
output_frames/frame_000.png
...
output_frames/frame_063.png
```

## Asset frames
Animated assets use zero-padded 3-digit frame indices:

```txt
assets/asteroid/frames/asteroid_000.png
assets/twinkle_large_glow/frames/twinkle_large_glow_000.png
assets/galaxy_edge_stars/frames/galaxy_edge_stars_000.png
```

## Masks
Each asset frame has a matching mask with the same numeric frame index.

```txt
assets/<asset-name>/masks/<asset-name>_mask_000.png
```

White pixels in a mask are active sprite pixels. Black pixels are transparent.

## Asteroid clearance masks
The asteroid includes a `clearance_masks/` directory.

These are NOT the visual sprite. They are used to erase the background under the asteroid before drawing the asteroid frame. This creates the black border/readability buffer around the asteroid.

## Static assets
Static assets still use a `_000.png` frame so code can treat static and animated assets consistently.
