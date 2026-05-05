# neko sprite pack

> **Available languages**: [English (current)](README.md) | [Italiano](README.it.md)

This directory is a **template**. It does not ship a `neko.png` image because:

- The classic oneko bitmaps were authored by Masayuki Koba in the early 1990s
  and are commonly described as public-domain (the README of
  [IreneKnapp/oneko](https://github.com/IreneKnapp/oneko) literally states
  "essentially PDS"), but **no canonical license file accompanies the bitmaps**
  upstream, so we don't redistribute them by default.
- The Felix the Cat / "MyFelix" assets are **proprietary** and must not be
  bundled. If you grew up with Felix screenmate and want him here, supply
  your own legally licensed art.

## How to install a sheet

1. Drop a 32×32 (or any size — adjust `frame_w` / `frame_h` in `sheet.conf`)
   PNG sprite sheet next to this file. Default file name: `neko.png`.
2. Edit `sheet.conf` to map animation names to grid coordinates (see file
   comments).
3. Point `sprite_dir` in your `aymm.conf` at this directory.

## Suggested sources

- Convert the bitmaps from oneko's `bitmaps/` directory yourself
  (xbm → png via ImageMagick: `convert in.xbm -negate out.png`).
- Use any sprite pack from your own collection that the license permits.
- Commission art under a license you control.

If you produce a sheet you have the right to redistribute under MIT-compatible
terms, a PR adding it under `assets/sprites/<name>/` is welcome.
