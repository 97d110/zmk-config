# Naming Conventions

- Output frames are in `output_frames/frame_000.png` through `frame_063.png`.
- Animated assets use zero-padded frame names, for example `asteroid_000.png`.
- Masks match the same numeric index.
- White pixels in a mask are active sprite pixels; black pixels are transparent.
- The asteroid also has `clearance_masks/`; erase the background under this mask before drawing the asteroid to create its black border.
- Static assets still use a `_000.png` frame so code can treat all assets consistently.
