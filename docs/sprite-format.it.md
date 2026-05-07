# Formato sprite sheet

> **Lingue disponibili**: [English](sprite-format.md) | [Italiano (corrente)](sprite-format.it.md)


Ogni sprite pack è una directory contenente:

- `<image>.png` — sprite sheet PNG, RGBA preferibile.
- `sheet.conf`  — manifest (file di testo key=value).

## Reference di sheet.conf

```
image=cat.png
frame_w=32
frame_h=32

anim.<nome>=<lista_frame>
```

`<lista_frame>` è una lista di coordinate `col,row` (0-indexed) separate
da `;` nella griglia definita da `frame_w` / `frame_h`. Aggiungi il
token letterale `mirror` per specchiare l'animazione orizzontalmente:

```
anim.run_w = 0,5 ; 1,5 ; mirror
```

## Nomi di animazione riconosciuti

Il resolver delle animazioni (`src/animation/Animation.cpp`) prova i
nomi seguenti, in ordine, con fallback su `idle` se manca un nome più
specifico:

| Stato             | Nome lookup        |
|-------------------|--------------------|
| Idle              | `idle`             |
| Sleep             | `sleep`            |
| Wake              | `wake`             |
| Scratch           | `scratch`          |
| DirectionChange   | `direction_change` |
| Run, direzione d  | `run_<d>`          |

Dove `<d>` è uno tra `n / ne / e / se / s / sw / w / nw`.
