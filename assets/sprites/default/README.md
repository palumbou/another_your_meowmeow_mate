# Sprite pack

> **Available languages**: [English (current)](README.md) | [Italiano](README.it.md)

This directory is the default location aymm looks at when `sprite_dir`
points here. It defines the **mapping** between the animation states the
state machine emits (`idle`, `sleep`, `wake`, `scratch`, `run_n`, `run_e`,
…) and frames inside a PNG sprite sheet.

## How to install your own sheet

1. Drop a PNG sprite sheet next to this file. The default file name read by
   `sheet.conf` is `cat.png` — change `image=` if you use a different name.
2. Edit `sheet.conf` to declare the tile size (`frame_w` / `frame_h`) and
   the per-animation list of `(col, row)` frames, plus the optional
   `mirror` flag for horizontally-flipped clips. Example:

   ```ini
   image=cat.png
   frame_w=32
   frame_h=32

   anim.idle  = 0,0
   anim.sleep = 0,1 ; 1,1
   anim.run_e = 0,5 ; 1,5
   anim.run_w = 0,5 ; 1,5 ; mirror
   ```

3. Run aymm with `sprite_dir=` (in the config file) or `--config` pointing
   at a config that sets it to this directory:

   ```sh
   aymm                                  # uses default config path
   aymm -c examples/aymm.conf            # or any custom config
   ```

If no PNG is present and `sprite_dir` is empty, aymm draws its built-in
procedural cat — no asset is required to use the program.

See [docs/sprite-format.md](../../../docs/sprite-format.md) for the full
manifest reference and the recognized animation names.
