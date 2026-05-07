# examples/

> **Lingue disponibili**: [English](README.md) | [Italiano (corrente)](README.it.md)

File di configurazione di esempio per i vari servizi con cui aymm
interagisce. La spiegazione discorsiva sta nel [README.md](../README.md)
del progetto; questa cartella è il posto canonico da cui prendere uno
snippet funzionante da incollare nelle tue config.

| File                          | Cos'è                                                                                                                                  |
|-------------------------------|----------------------------------------------------------------------------------------------------------------------------------------|
| `aymm.conf`                   | Config di reference per aymm stesso. Copialo in `~/.config/aymm/aymm.conf` e modificalo, o passalo ad aymm con `-c examples/aymm.conf`. Ogni chiave riconosciuta è documentata inline. |
| `hyprland-autostart.conf`     | Snippet da incollare in `~/.config/hypr/hyprland.conf` per l'autostart e una layer rule.                                               |
| `waybar-module.jsonc`         | Modulo `custom/aymm` da inserire in `~/.config/waybar/config`. Legge JSON da `aymm waybar-status` e cabla le azioni di click.          |

Per provare uno snippet senza copiarlo:

```sh
aymm -c examples/aymm.conf
```
