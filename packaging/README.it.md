# packaging/

> **Lingue disponibili**: [English](README.md) | [Italiano (corrente)](README.it.md)

Scaffolding distro-specifico per pacchettizzare aymm per gli utenti finali.

## Layout

| Path                            | Formato / target                                                            |
|---------------------------------|-----------------------------------------------------------------------------|
| `desktop/aymm.desktop`          | Entry `.desktop` Freedesktop — integrazione nel launcher.                   |
| `systemd/aymm.service`          | Unit systemd **user**; usa con `systemctl --user enable aymm`.              |
| `rpm/aymm.spec`                 | Spec RPM per Fedora / RHEL / openSUSE-style.                                |
| `deb/control`, `deb/rules`      | Metadata + rules del source-package Debian/Ubuntu.                          |

Il flake Nix nella root del progetto (`flake.nix`) copre NixOS —
installa con `nix profile install .#aymm` o builda in standalone con
`nix build .#aymm`.

## Nota sullo sprite del gatto

aymm spedisce un **gatto custom disegnato interamente con path Cairo**
come visualizzazione di default — non c'è alcun asset immagine esterno
nel binario. Non abbiamo riutilizzato le bitmap classiche di
[oneko](https://github.com/IreneKnapp/oneko) perché upstream non hanno
un file di licenza chiaro (il README le descrive come "essentially
public domain", ma l'assenza di una licenza vera complica la
ridistribuzione). Il gatto procedurale aggira completamente il
problema: è arte nostra, permissive in stile MIT anche se il repo è
sotto CC BY-NC 4.0 (vedi [LICENSE](../LICENSE)).

Se vuoi un gatto diverso (o uno screenmate stile Felix, o altro), metti
uno sprite sheet PNG in `assets/sprites/default/` e fai puntare la
chiave `sprite_dir` della config. Vedi
[../docs/sprite-format.it.md](../docs/sprite-format.it.md).
