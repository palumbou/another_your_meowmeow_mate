# Hyprneko

> **Lingue disponibili**: [English](README.md) | [Italiano (corrente)](README.it.md)

Un desktop pet Wayland-native che insegue il cursore su Hyprland — ispirato
a oneko, senza X11/XWayland nello stack.

## Stato

Versione **0.1**. La build compila, il binario gira, CLI e output Waybar
funzionano. Overlay Wayland + loop di cursor-chase sono cablati end-to-end su
Hyprland; se non fornisci uno sprite sheet, il pet viene renderizzato come
cerchio arancione così puoi validare il chase loop anche senza arte.

## Perché un altro oneko

I fork di oneko esistenti (es. [IreneKnapp/oneko](https://github.com/IreneKnapp/oneko),
[glreno/oneko](https://github.com/glreno/oneko)) puntano direttamente a X11,
e il port Java passa comunque da XWayland sui Linux moderni. Hyprneko è
costruito su `wlr-layer-shell-unstable-v1` e `wayland-client`, quindi
compone come overlay Wayland reale senza X server di mezzo. L'architettura
si ispira a [furudbat/wayland-vpets](https://github.com/furudbat/wayland-vpets)
(C++/CMake, layer-shell, Cairo).

Il vincolo Wayland interessante è che la **posizione del cursore non è mai
trasmessa ai client generici**, quindi il chase loop ha bisogno di una
sorgente fuori-banda. Hyprneko la astrae dietro `CursorProvider`. Il
backend di default parla con il socket IPC di Hyprland (senza fork di
`hyprctl`). Vedi [docs/cursor-providers.md](docs/cursor-providers.md).

## Build

### Build rapido

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/hyprneko --version
```

Pacchetti di sistema richiesti:

| Distro   | Comando |
|----------|---------|
| NixOS    | `nix develop` (usa il flake), oppure `nix-shell -p cmake pkg-config wayland wayland-protocols wayland-scanner cairo gcc` |
| Fedora   | `sudo dnf install cmake gcc-c++ pkgconf-pkg-config wayland-devel wayland-protocols-devel cairo-devel` |
| Ubuntu   | `sudo apt install build-essential cmake pkg-config libwayland-dev wayland-protocols libwayland-bin libcairo2-dev` |

### Nix flake

```sh
nix build .#hyprneko          # build
nix run  .#hyprneko           # esegui
nix develop                   # shell di sviluppo con tutte le dipendenze
nix build .#hyprneko-queen    # build con override Queen persona
```

## Configurazione

Copia `examples/hyprneko.conf` in `$XDG_CONFIG_HOME/hyprneko/hyprneko.conf`
e personalizza. La reference completa è nel file di esempio. Estratto:

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

Gli sprite sheet sono opzionali. Per usarne uno, imposta `sprite_dir=` a una
directory che contenga `sheet.conf` e il PNG corrispondente. Vedi
[docs/sprite-format.md](docs/sprite-format.md) e
[assets/sprites/neko/README.it.md](assets/sprites/neko/README.it.md) — incluse
le note di licenza sul perché nessun PNG è incluso nel repo.

## Autostart Hyprland

```
# ~/.config/hypr/hyprland.conf
exec-once = hyprneko
layerrule = blur, hyprneko
layerrule = ignorezero, hyprneko
```

Vedi [examples/hyprland-autostart.conf](examples/hyprland-autostart.conf).

## Pomodoro

```sh
hyprneko pomodoro start         # inizia una sessione di focus
hyprneko pomodoro pause
hyprneko pomodoro resume
hyprneko pomodoro skip          # salta alla fase successiva
hyprneko pomodoro stop
hyprneko pomodoro status        # leggibile umano
```

Fasi riportate da CLI / Waybar JSON: `focus`, `break`, `long_break`,
`paused`, `stopped`. Durante il focus il pet rallenta e resta tranquillo;
durante la pausa torna a velocità e idle distance di base.

## Modulo Waybar

```jsonc
"custom/hyprneko": {
  "format": "{}",
  "return-type": "json",
  "interval": 1,
  "exec": "hyprneko waybar-status",
  "on-click":        "hyprneko pomodoro toggle",
  "on-click-right":  "hyprneko toggle",
  "on-click-middle": "hyprneko quit"
}
```

Payload prodotto:

```json
{ "text": "🐈 24:12", "tooltip": "Pomodoro: focus session — 24:12 remaining",
  "class": "focus", "percentage": 80 }
```

Se nessun daemon è in esecuzione, `hyprneko waybar-status` produce comunque
un payload `stopped` valido così Waybar non si lamenta.

## Sintesi CLI

```
hyprneko [run]                Avvia il daemon del pet (default).
hyprneko toggle               Mostra/nascondi su un daemon esistente.
hyprneko quit                 Termina il daemon esistente.
hyprneko status               Stato leggibile umano.
hyprneko waybar-status        JSON Waybar one-shot.
hyprneko pomodoro <cmd>       start | pause | resume | stop | skip | toggle | status
```

La CLI parla col daemon via socket AF_UNIX in
`$XDG_RUNTIME_DIR/hyprneko/control.sock`.

## Limiti e non-obiettivi

- **Backend cursore solo Hyprland in v1.** I provider wlroots e evdev sono
  stub riservati.
- **Niente sprite proprietari.** Gli asset Felix the Cat non sono inclusi e
  non lo saranno mai. Vedi
  [assets/sprites/neko/README.it.md](assets/sprites/neko/README.it.md).
- **Niente logging dei tasti.** Il backend evdev (quando implementato)
  filtrerà solo `EV_REL`/`EV_ABS`/`BTN_*`.

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
- `feature/queen-bianca-mode` — override opt-in che saluta l'utente come
  "Queen" se lo username di sistema è una variante di Bianca. Vive sul suo
  branch di proposito, non viene merged in master.

## Licenza

MIT. Vedi [LICENSE](LICENSE).

L'XML di `wlr-layer-shell-unstable-v1` è vendorizzato sotto la stessa
licenza permissiva tipo MIT dell'upstream wlr-protocols.
