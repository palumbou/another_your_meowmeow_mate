# examples/

> **Available languages**: [English (current)](README.md) | [Italiano](README.it.md)

Sample configuration files for the various services aymm interacts with.
The full prose is in the project [README.md](../README.md); this folder
is the canonical place to grab a working snippet you can drop into your
own configs.

| File                          | What it is                                                          |
|-------------------------------|---------------------------------------------------------------------|
| `aymm.conf`                   | Reference config for aymm itself. Copy to `~/.config/aymm/aymm.conf` and edit, or pass to aymm with `-c examples/aymm.conf`. Every recognized key is documented inline. |
| `hyprland-autostart.conf`     | Snippet to paste into `~/.config/hypr/hyprland.conf` for autostart and a layer rule.                                          |
| `waybar-module.jsonc`         | Drop-in `custom/aymm` module for `~/.config/waybar/config`. Reads JSON from `aymm waybar-status` and wires click actions.    |

To try a snippet without copying:

```sh
aymm -c examples/aymm.conf
```
