# Hyprneko

> **Available languages**: [English (current)](README.md) | [Italiano](README.it.md)

A Wayland-native desktop pet that chases your cursor under Hyprland —
inspired by oneko, with no X11/XWayland in the stack.

```
                 ╱╲___╱╲
                ( -    - )       chase: cursor
                 |  ⌒  |         layer:  overlay
                 ╰─────╯         pomodoro: optional
```

## Status

This is **v0.1**. The build is green, the binary runs, the CLI and Waybar
output work. The Wayland overlay + cursor chase loop is wired end-to-end
on Hyprland; if you don't supply a sprite sheet, the pet renders as an
orange circle so you can validate the chase loop without art.

## Why another oneko

Most existing forks of oneko (e.g. [IreneKnapp/oneko](https://github.com/IreneKnapp/oneko),
[glreno/oneko](https://github.com/glreno/oneko)) target X11 directly, and the
Java port also goes through XWayland on modern Linux. Hyprneko is built on
top of `wlr-layer-shell-unstable-v1` and `wayland-client`, so it composites
as a real Wayland overlay layer with no X server involved. The architecture
is inspired by [furudbat/wayland-vpets](https://github.com/furudbat/wayland-vpets)
(C++/CMake, layer-shell, Cairo).

The interesting Wayland constraint is that **cursor position is never
broadcast to arbitrary clients**, so the chase loop needs an out-of-band
source. Hyprneko abstracts that behind `CursorProvider`. The default backend
talks to Hyprland's IPC socket (no `hyprctl` fork). See
[docs/cursor-providers.md](docs/cursor-providers.md).

## Build

### Quick build (any distro with the deps installed)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/hyprneko --version
```

Required system packages:

| Distro   | Command |
|----------|---------|
| NixOS    | `nix develop` (uses the flake), or `nix-shell -p cmake pkg-config wayland wayland-protocols wayland-scanner cairo gcc` |
| Fedora   | `sudo dnf install cmake gcc-c++ pkgconf-pkg-config wayland-devel wayland-protocols-devel cairo-devel` |
| Ubuntu   | `sudo apt install build-essential cmake pkg-config libwayland-dev wayland-protocols libwayland-bin libcairo2-dev` |

### Nix flake

```sh
nix build .#hyprneko          # build
nix run  .#hyprneko           # run
nix develop                   # dev shell with all build deps
nix build .#hyprneko-queen    # build with the Queen persona override
```

## Configure

Drop `examples/hyprneko.conf` into `$XDG_CONFIG_HOME/hyprneko/hyprneko.conf`
and tweak. The full reference is in the example file. Highlights:

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

Sprite sheets are optional. To use one, point `sprite_dir=` at a directory
that contains a `sheet.conf` and the matching PNG. See
[docs/sprite-format.md](docs/sprite-format.md) and
[assets/sprites/neko/README.md](assets/sprites/neko/README.md) — including
the licensing notes about why no PNG ships with the repo.

## Hyprland autostart

```
# ~/.config/hypr/hyprland.conf
exec-once = hyprneko
layerrule = blur, hyprneko
layerrule = ignorezero, hyprneko
```

See [examples/hyprland-autostart.conf](examples/hyprland-autostart.conf).

## Pomodoro

```sh
hyprneko pomodoro start         # start a focus session
hyprneko pomodoro pause
hyprneko pomodoro resume
hyprneko pomodoro skip          # jump to next phase
hyprneko pomodoro stop
hyprneko pomodoro status        # human-readable
```

Phases reported in CLI / Waybar JSON: `focus`, `break`, `long_break`,
`paused`, `stopped`. During focus the pet slows down and stays calm; during
break it returns to its base speed and idle distance.

## Waybar module

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

Output payload:

```json
{ "text": "🐈 24:12", "tooltip": "Pomodoro: focus session — 24:12 remaining",
  "class": "focus", "percentage": 80 }
```

If no daemon is running, `hyprneko waybar-status` emits a valid `stopped`
payload so Waybar stays happy.

## CLI summary

```
hyprneko [run]                Start the pet daemon (default).
hyprneko toggle               Show/hide on a running daemon.
hyprneko quit                 Tell the running daemon to exit.
hyprneko status               Human-readable status.
hyprneko waybar-status        Single-shot Waybar JSON.
hyprneko pomodoro <cmd>       start | pause | resume | stop | skip | toggle | status
```

The CLI talks to the daemon over an AF_UNIX socket at
`$XDG_RUNTIME_DIR/hyprneko/control.sock`.

## Limits & non-goals

- **Cursor backend is Hyprland-only in v1.** The wlroots and evdev providers
  are reserved stubs.
- **No proprietary sprites.** Felix the Cat assets are not bundled and never
  will be. See [assets/sprites/neko/README.md](assets/sprites/neko/README.md).
- **No keystroke logging.** The evdev backend (when implemented) will
  whitelist `EV_REL`/`EV_ABS`/`BTN_*` only.

## Layout

```
src/
  app/            App.cpp, Config.cpp        run loop + config parser
  wayland/        OverlaySurface.cpp         layer-shell + wl_shm + Cairo
  cursor/         HyprlandCursorProvider     IPC socket, NullCursorProvider, evdev stub
  pet/            Pet, PetStateMachine,      neko-style FSM + behaviors
                  CursorChaseBehavior,
                  PomodoroBehavior
  animation/      SpriteSheet, Animation     PNG loader + (state,direction)→frames
  pomodoro/       PomodoroTimer              pure timer
  waybar/         WaybarStatus               JSON formatter
  persona/        UserPersona                neutral, queen variant on feature branch
  cli/            Cli, ControlSocket         argv parser + AF_UNIX daemon channel
```

## Branches

- `master` — neutral persona, the supported main line.
- `feature/queen-bianca-mode` — opt-in persona override that greets the user
  as "Queen" when the system username is a Bianca-variant. Lives on its own
  branch on purpose; not merged into master.

## License

MIT. See [LICENSE](LICENSE).

The XML for `wlr-layer-shell-unstable-v1` is vendored under the same MIT-like
permissive license as the upstream wlr-protocols.
