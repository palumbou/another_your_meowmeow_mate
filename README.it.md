# Another Your MeowMeow Mate

> **Lingue disponibili**: [English](README.md) | [Italiano (corrente)](README.it.md)

Un desktop pet Wayland-native che insegue il cursore su Hyprland — ispirato
a oneko, senza X11/XWayland nello stack.

## Stato

**v0.1**. Build verde, CLI e output Waybar funzionanti, overlay Wayland
e loop di cursor-chase cablati end-to-end su Hyprland. Out of the box,
senza alcun asset sprite esterno, il pet è un piccolo gatto bianco
disegnato con path Cairo — testa, orecchie, occhi (con pupille), coda
che oscilla, animazione delle zampe, posa sleep raggomitolata in una
cuccia rosa con fiocco e cuoricini.

## Perché un pet Wayland-native

I fork di oneko esistenti (es. [IreneKnapp/oneko](https://github.com/IreneKnapp/oneko),
[glreno/oneko](https://github.com/glreno/oneko)) puntano direttamente a X11,
e il port Java passa comunque da XWayland sui Linux moderni. aymm è
costruito su `wlr-layer-shell-unstable-v1` e `wayland-client`, quindi
compone come overlay Wayland reale senza X server di mezzo. L'architettura
si ispira a [furudbat/wayland-vpets](https://github.com/furudbat/wayland-vpets)
(C++/CMake, layer-shell, Cairo).

Il vincolo Wayland interessante è che la **posizione del cursore non è mai
trasmessa ai client generici**, quindi il chase loop ha bisogno di una
sorgente fuori-banda. aymm la astrae dietro `CursorProvider`. Il backend
di default parla con il socket IPC di Hyprland (senza fork di
`hyprctl`). Vedi [docs/cursor-providers.it.md](docs/cursor-providers.it.md).

## Build

### Build rapido

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/aymm --version
```

Pacchetti di sistema richiesti:

| Distro   | Comando |
|----------|---------|
| NixOS    | `nix develop` (usa il flake), oppure `nix-shell -p cmake pkg-config wayland wayland-protocols wayland-scanner cairo gcc` |
| Fedora   | `sudo dnf install cmake gcc-c++ pkgconf-pkg-config wayland-devel wayland-protocols-devel cairo-devel` |
| Ubuntu   | `sudo apt install build-essential cmake pkg-config libwayland-dev wayland-protocols libwayland-bin libcairo2-dev` |

### Nix

Due opzioni equivalenti su NixOS / sistemi con nix:

```sh
# Flake (consigliato)
nix build .#aymm          # build
nix run  .#aymm           # esegui
nix develop               # shell di sviluppo con tutte le dipendenze
nix build .#aymm-queen    # build con override Queen persona

# shell.nix (no flake)
nix-shell                 # legge shell.nix dalla root del progetto
```

## Configurazione

Di default `aymm` cerca `$XDG_CONFIG_HOME/aymm/aymm.conf` (tipicamente
`~/.config/aymm/aymm.conf`). Il file `examples/aymm.conf` è un template
completamente commentato — copialo lì e modificalo, oppure punta `aymm` a
qualsiasi path con `-c` / `--config`:

```sh
aymm -c examples/aymm.conf            # prova l'esempio senza copiarlo
aymm --config /etc/aymm/site.conf     # config a livello sistema
aymm --config=~/aymm.conf             # accetta anche la forma con =
```

La reference completa è nel file di esempio. Estratto:

```ini
animation_name=neko
behavior=cursor_chase
cursor_source=hyprland
movement_speed=8
idle_distance=24
overlay_layer=overlay
monitor=auto

enable_pomodoro=1
pomodoro_study_minutes=25
pomodoro_break_minutes=5
```

Gli sprite sheet sono opzionali — il gatto procedurale è il default.
Per sostituirlo con la tua arte, metti un PNG in
`assets/sprites/default/` (accanto a `sheet.conf`) e imposta
`sprite_dir=` nella config a quel path. Vedi
[docs/sprite-format.it.md](docs/sprite-format.it.md) e
[assets/sprites/default/README.it.md](assets/sprites/default/README.it.md).

## Autostart Hyprland

```
# ~/.config/hypr/hyprland.conf
exec-once = aymm
layerrule = blur, aymm
layerrule = ignorezero, aymm
```

Vedi [examples/hyprland-autostart.conf](examples/hyprland-autostart.conf).

## Pomodoro

Avvia una sessione con un solo comando (non serve daemon prima):

```sh
aymm --pomodoro                                       # con i default
aymm --pomodoro --study-minutes 50 --break-minutes 10 # durate custom
aymm --pomodoro --focus-corner top-left               # parcheggia in alto a sx
```

Oppure controlla un daemon già in esecuzione:

```sh
aymm pomodoro start         # inizia una sessione di focus
aymm pomodoro pause
aymm pomodoro resume
aymm pomodoro skip          # salta alla fase successiva
aymm pomodoro stop
aymm pomodoro status        # leggibile umano
```

I flag CLI sovrascrivono le rispettive chiavi di config:

| Flag                                 | Chiave di config                         |
|--------------------------------------|------------------------------------------|
| `--study-minutes N`                  | `pomodoro_study_minutes`                 |
| `--break-minutes N`                  | `pomodoro_break_minutes`                 |
| `--long-break-minutes N`             | `pomodoro_long_break_minutes`            |
| `--sessions-before-long-break N`     | `pomodoro_sessions_before_long_break`    |
| `--focus-corner C`                   | `pomodoro_focus_corner`                  |

Fasi riportate da CLI / Waybar JSON: `focus`, `break`, `long_break`,
`paused`, `stopped`. In focus il gatto dorme nella cuccia nell'angolo
configurato; in break si sveglia e ricomincia a inseguire il cursore.

## Modulo Waybar

```jsonc
"custom/aymm": {
  "format": "{}",
  "return-type": "json",
  "interval": 1,
  "exec": "aymm waybar-status",
  "on-click":        "aymm pomodoro toggle",
  "on-click-right":  "aymm toggle",
  "on-click-middle": "aymm quit"
}
```

Payload prodotto:

```json
{ "text": "🐈 24:12", "tooltip": "Pomodoro: focus session — 24:12 remaining",
  "class": "focus", "percentage": 80 }
```

Se nessun daemon è in esecuzione, `aymm waybar-status` produce comunque
un payload `stopped` valido così Waybar non si lamenta.

## Sintesi CLI

```
aymm [run]                Avvia il daemon del pet (default).
aymm toggle               Mostra/nascondi su un daemon esistente.
aymm quit                 Termina il daemon esistente.
aymm status               Stato leggibile umano.
aymm waybar-status        JSON Waybar one-shot.
aymm pomodoro <cmd>       start | pause | resume | stop | skip | toggle | status
```

La CLI parla col daemon via socket AF_UNIX in
`$XDG_RUNTIME_DIR/aymm/control.sock`.

## Compositor supportati

| Compositor                | Inseguimento | Note                                                                                                            |
|---------------------------|--------------|-----------------------------------------------------------------------------------------------------------------|
| Hyprland                  | ✅ funziona   | usa `hyprctl cursorpos` via socket IPC — nessun permesso speciale                                                |
| KDE Plasma 6 / KWin       | ❌ no         | KWin usa libinput che fa `EVIOCGRAB` esclusivo sui device input, quindi le `read()` di aymm non vedono niente. Il gatto **viene comunque renderizzato** nella cuccia; semplicemente non può inseguire il mouse. KDE non ha un'API pubblica per la posizione del cursore. |
| sway / wlroots generico   | ⚠️ non testato | il layer-shell renderizza, `evdev` funziona *se* l'input backend del compositor non fa grab esclusivo (libinput sì, quindi la maggior parte dei wlroots ha lo stesso problema di KDE). L'IPC stile Hyprland non c'è. |
| GNOME / Mutter            | ❌ niente overlay | Mutter **non** espone `wlr-layer-shell` — aymm esce all'avvio con `compositor missing required globals`. Il gatto non appare mai. Su GNOME, installa un compositor wlroots (Hyprland, sway) per usare aymm. |

**Consiglio pratico su Fedora**: installa Hyprland (`sudo dnf install hyprland`)
e fai login in sessione Hyprland per avere la chase loop. KDE Plasma è
supportato nella misura in cui il gatto viene disegnato correttamente, ma
la chase loop è un limite a livello di stack Wayland che non possiamo
aggirare senza modifiche lato compositor.

## Limiti e non-obiettivi

- **Niente logging dei tasti.** Il backend evdev whitelist-a solo
  `EV_REL`/`EV_ABS` — mai key code. Vedi
  [docs/cursor-providers.it.md](docs/cursor-providers.it.md).

## Layout

```
src/
  app/            App.cpp, Config.cpp        run loop + parser config
  wayland/        OverlaySurface.cpp         layer-shell + wl_shm + Cairo
  cursor/         HyprlandCursorProvider     socket IPC, NullCursorProvider, stub evdev
  pet/            Pet, PetStateMachine,      FSM stile neko + comportamenti
                  CursorChaseBehavior,
                  PomodoroBehavior
  animation/      SpriteSheet, Animation     loader PNG + (stato,direzione)→frame
  pomodoro/       PomodoroTimer              timer puro
  waybar/         WaybarStatus               formatter JSON
  persona/        UserPersona                neutra; variante queen sul branch dedicato
  cli/            Cli, ControlSocket         parser argv + canale AF_UNIX
```

## Branch

- `master` — persona neutra, mainline supportata.
- `feature/queen-bianca-mode` (**questo branch**) — l'override Queen persona
  è compilato di default (`-DAYMM_QUEEN_MODE=ON`). L'override si attiva
  **a runtime, solo** quando lo username Linux corrente corrisponde
  esattamente a `bianca` o al suo equivalente inglese `white`
  (case-insensitive, niente diminutivi né affissi). Gli altri utenti
  vedono la persona neutra in modo trasparente — nessun rischio di
  salutare per sbaglio un utente non coinvolto come Queen. Differenze
  in output:

  | dove      | neutra                        | queen                                    |
  |-----------|-------------------------------|------------------------------------------|
  | saluto    | `Welcome back`                | `Welcome back, Queen`                    |
  | tooltip   | `Pomodoro: focus session — …` | `Queen's focus session — …`              |
  | onorifico | (vuoto)                       | `, Queen` (riservato per le notifiche)   |

  Questo branch è intenzionalmente **non** mergiato in master e resta opt-in.

## Licenza

[CC BY-NC 4.0](LICENSE) — Creative Commons Attribution-NonCommercial
4.0 International. Puoi usare, condividere e modificare per scopi non
commerciali con attribuzione. Per uso commerciale contattare prima.

L'XML di `wlr-layer-shell-unstable-v1` è vendorizzato sotto la stessa
licenza permissiva tipo MIT dell'upstream wlr-protocols.
