# Sprite pack neko

> **Lingue disponibili**: [English](README.md) | [Italiano (corrente)](README.it.md)

Questa directory è un **template**. Non contiene `neko.png` perché:

- Le bitmap originali di oneko (Masayuki Koba, primi anni '90) sono
  generalmente considerate di pubblico dominio (il README di
  [IreneKnapp/oneko](https://github.com/IreneKnapp/oneko) dichiara
  letteralmente "essentially PDS"), ma **upstream non c'è un file di licenza
  canonico**, quindi non le ridistribuiamo di default.
- Gli asset Felix the Cat / "MyFelix" sono **proprietari** e non vanno
  inclusi. Se vuoi Felix qui, fornisci tu un'arte regolarmente licenziata.

## Come installare uno sheet

1. Metti un PNG sprite sheet 32×32 (o di qualsiasi dimensione — basta
   aggiornare `frame_w` / `frame_h` in `sheet.conf`) accanto a questo file.
   Nome di default: `neko.png`.
2. Modifica `sheet.conf` mappando i nomi delle animazioni alle coordinate
   della griglia (vedi commenti nel file).
3. Imposta `sprite_dir` nel tuo `aymm.conf` puntando a questa directory.

## Sorgenti consigliate

- Converti le bitmap dalla directory `bitmaps/` di oneko
  (xbm → png con ImageMagick: `convert in.xbm -negate out.png`).
- Usa uno sprite pack della tua collezione, se la licenza lo permette.
- Commissiona arte sotto licenza che controlli.

Se produci uno sheet che hai diritto di ridistribuire sotto licenza
compatibile con MIT, una PR sotto `assets/sprites/<nome>/` è benvenuta.
