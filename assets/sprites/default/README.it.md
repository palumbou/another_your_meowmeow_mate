# Sprite pack

> **Lingue disponibili**: [English](README.md) | [Italiano (corrente)](README.it.md)

Questa directory è il path che aymm guarda quando `sprite_dir` la
referenzia. Definisce il **mapping** tra gli stati che la state machine
emette (`idle`, `sleep`, `wake`, `scratch`, `run_n`, `run_e`, …) e i frame
all'interno di uno sprite sheet PNG.

## Come installare uno sheet

1. Metti un PNG sprite sheet accanto a questo file. Il nome di default
   letto da `sheet.conf` è `cat.png` — cambia `image=` se usi un altro
   nome.
2. Modifica `sheet.conf` dichiarando la dimensione del tile (`frame_w` /
   `frame_h`) e la lista per animazione di frame `(col, row)`, più il
   flag opzionale `mirror` per le clip specchiate orizzontalmente.
   Esempio:

   ```ini
   image=cat.png
   frame_w=32
   frame_h=32

   anim.idle  = 0,0
   anim.sleep = 0,1 ; 1,1
   anim.run_e = 0,5 ; 1,5
   anim.run_w = 0,5 ; 1,5 ; mirror
   ```

3. Lancia aymm con `sprite_dir=` (nel file di config) o `--config` che
   punta a una config che lo imposta a questa directory:

   ```sh
   aymm                                  # usa il path di config di default
   aymm -c examples/aymm.conf            # o qualsiasi config custom
   ```

Se non c'è nessun PNG e `sprite_dir` è vuoto, aymm disegna il suo gatto
procedurale built-in — non serve alcun asset per usare il programma.

Vedi [docs/sprite-format.md](../../../docs/sprite-format.md) per la
reference completa del manifest e i nomi di animazione riconosciuti.
