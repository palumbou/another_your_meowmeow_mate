# Sprite sheet format

> **Available languages**: [English (current)](sprite-format.md) | [Italiano](sprite-format.it.md)


Every sprite pack is a directory containing:

- `<image>.png` — sprite sheet PNG, RGBA preferred.
- `sheet.conf`  — manifest (key=value text file).

## sheet.conf reference

```
image=cat.png
frame_w=32
frame_h=32

anim.<name>=<frame_list>
```

`<frame_list>` is a semicolon-separated list of `col,row` tile coordinates
(0-indexed) into the grid defined by `frame_w` / `frame_h`. Append the
literal token `mirror` to mirror the animation horizontally:

```
anim.run_w = 0,5 ; 1,5 ; mirror
```

## Recognized animation names

The animation resolver (`src/animation/Animation.cpp`) tries the following
names, in order, falling back to `idle` if a more specific one is missing:

| State             | Lookup name        |
|-------------------|--------------------|
| Idle              | `idle`             |
| Sleep             | `sleep`            |
| Wake              | `wake`             |
| Scratch           | `scratch`          |
| DirectionChange   | `direction_change` |
| Run, direction d  | `run_<d>`          |

Where `<d>` is one of `n / ne / e / se / s / sw / w / nw`.
