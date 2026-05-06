# Another Your MeowMeow Mate

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
Java port also goes through XWayland on modern Linux. Another Your MeowMeow Mate is built on
top of `wlr-layer-shell-unstable-v1` and `wayland-client`, so it composites
as a real Wayland overlay layer with no X server involved. The architecture
is inspired by [furudbat/wayland-vpets](https://github.com/furudbat/wayland-vpets)
(C++/CMake, layer-shell, Cairo).

The interesting Wayland constraint is that **cursor position is never
broadcast to arbitrary clients**, so the chase loop needs an out-of-band
source. Another Your MeowMeow Mate abstracts that behind `CursorProvider`. The default backend
talks to Hyprland's IPC socket (no `hyprctl` fork). See
[docs/cursor-providers.md](docs/cursor-providers.md).

## Build

### Quick build (any distro with the deps installed)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/aymm --version
```

Required system packages:

| Distro   | Command |
|----------|---------|
| NixOS    | `nix develop` (uses the flake), or `nix-shell -p cmake pkg-config wayland wayland-protocols wayland-scanner cairo gcc` |
| Fedora   | `sudo dnf install cmake gcc-c++ pkgconf-pkg-config wayland-devel wayland-protocols-devel cairo-devel` |
| Ubuntu   | `sudo apt install build-essential cmake pkg-config libwayland-dev wayland-protocols libwayland-bin libcairo2-dev` |

### Nix flake

```sh
nix build .#aymm          # build
nix run  .#aymm           # run
nix develop                   # dev shell with all build deps
nix build .#aymm-queen    # build with the Queen persona override
```

## Configure

By default `aymm` looks for `$XDG_CONFIG_HOME/aymm/aymm.conf` (typically
`~/.config/aymm/aymm.conf`). The shipped `examples/aymm.conf` is a fully
documented template — drop it in that location and edit, or point `aymm`
at any path with `-c` / `--config`:

```sh
aymm -c examples/aymm.conf            # try the example without copying
aymm --config /etc/aymm/site.conf     # site-wide config
aymm --config=~/aymm.conf             # = form is also accepted
```

The full reference is in the example file. Highlights:

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
exec-once = aymm
layerrule = blur, aymm
layerrule = ignorezero, aymm
```

See [examples/hyprland-autostart.conf](examples/hyprland-autostart.conf).

## Pomodoro

Run a session in one command (no daemon needed up front):

```sh
aymm --pomodoro                                       # use the defaults
aymm --pomodoro --study-minutes 50 --break-minutes 10 # custom durations
aymm --pomodoro --focus-corner top-left               # park in top-left
```

Or control an already-running daemon:

```sh
aymm pomodoro start         # start a focus session
aymm pomodoro pause
aymm pomodoro resume
aymm pomodoro skip          # jump to next phase
aymm pomodoro stop
aymm pomodoro status        # human-readable
```

CLI flags override the matching config keys:

| Flag                                 | Config key                              |
|--------------------------------------|------------------------------------------|
| `--study-minutes N`                  | `pomodoro_study_minutes`                 |
| `--break-minutes N`                  | `pomodoro_break_minutes`                 |
| `--long-break-minutes N`             | `pomodoro_long_break_minutes`            |
| `--sessions-before-long-break N`     | `pomodoro_sessions_before_long_break`    |
| `--focus-corner C`                   | `pomodoro_focus_corner`                  |

Phases reported in CLI / Waybar JSON: `focus`, `break`, `long_break`,
`paused`, `stopped`. During focus the pet sleeps in its basket in the
configured corner; during break it wakes up and chases the cursor again.

## Waybar module

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

Output payload:

```json
{ "text": "🐈 24:12", "tooltip": "Pomodoro: focus session — 24:12 remaining",
  "class": "focus", "percentage": 80 }
```

If no daemon is running, `aymm waybar-status` emits a valid `stopped`
payload so Waybar stays happy.

## CLI summary

```
aymm [run]                Start the pet daemon (default).
aymm toggle               Show/hide on a running daemon.
aymm quit                 Tell the running daemon to exit.
aymm status               Human-readable status.
aymm waybar-status        Single-shot Waybar JSON.
aymm pomodoro <cmd>       start | pause | resume | stop | skip | toggle | status
```

The CLI talks to the daemon over an AF_UNIX socket at
`$XDG_RUNTIME_DIR/aymm/control.sock`.

## Compositor support

| Compositor                | Cursor chase | Notes                                                                                                            |
|---------------------------|--------------|------------------------------------------------------------------------------------------------------------------|
| Hyprland                  | ✅ works      | uses `hyprctl cursorpos` over the IPC socket — no permissions needed                                             |
| KDE Plasma 6 / KWin       | ❌ no         | KWin uses libinput which `EVIOCGRAB`s the input devices exclusively, so aymm's evdev reads return zero events. The cat still **renders** in the basket; it just can't follow the mouse. There is no public KDE API for cursor position. |
| sway / generic wlroots    | ⚠️ untested  | layer-shell renders fine; `evdev` works *if* the compositor's input backend doesn't grab exclusively (libinput does, so most wlroots compositors hit the same KDE issue). The Hyprland-style IPC is not exposed.                |

**Practical advice on Fedora**: install Hyprland (`sudo dnf install hyprland`)
and log into a Hyprland session for the chase loop. KDE Plasma is supported
to the extent that the cat is drawn correctly, but the chase loop is a
Wayland-stack-level limitation we can't work around without compositor-side
changes.

## Limits & non-goals

- **No proprietary sprites.** Felix the Cat assets are not bundled and never
  will be. See [assets/sprites/neko/README.md](assets/sprites/neko/README.md).
- **No keystroke logging.** The evdev backend whitelists `EV_REL`/`EV_ABS`
  only — never key codes. See [docs/cursor-providers.md](docs/cursor-providers.md).

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
- `feature/queen-bianca-mode` (**this branch**) — Queen persona override
  is compiled in (`-DAYMM_QUEEN_MODE=ON` by default). The override
  activates **at runtime only** when the current Linux username matches
  exactly `bianca` or its English equivalent `white` (case-insensitive,
  no nicknames or affixes). Every other user gets the neutral persona
  transparently — there is no chance of greeting an unrelated user as Queen.
  Output difference, when active:

  | location  | neutral                       | queen                                    |
  |-----------|-------------------------------|------------------------------------------|
  | greeting  | `Welcome back`                | `Welcome back, Queen`                    |
  | tooltip   | `Pomodoro: focus session — …` | `Queen's focus session — …`              |
  | honorific | (empty)                       | `, Queen` (used in future notifications) |

  This branch is intentionally **not** merged into master and stays opt-in.

## License

MIT. See [LICENSE](LICENSE).

The XML for `wlr-layer-shell-unstable-v1` is vendored under the same MIT-like
permissive license as the upstream wlr-protocols.
