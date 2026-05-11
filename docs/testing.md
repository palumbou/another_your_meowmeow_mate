# How to test aymm

> **Available languages**: [English (current)](testing.md) | [Italiano](testing.it.md)


This page covers three things:

1. Quick smoke tests that don't need a Wayland session at all.
2. The full visual test under Hyprland (cursor chase + multi-monitor +
   notifications).
3. What's in the prebuilt tarballs and on which systems they actually run.

## 1. Smoke tests (no Hyprland required)

These work in a plain shell on any Linux:

```sh
./bin/aymm --version
./bin/aymm --help
./bin/aymm waybar-status        # JSON, even with no daemon
USER=bianca ./bin/aymm waybar-status   # only meaningful on the queen branch
```

Expected:
- `--version` prints `aymm 0.1.0+<hash> (<commit-date>)`
- `waybar-status` prints valid JSON ending in `"percentage":0`
- On the queen branch: `USER=bianca` or `USER=White` → tooltip says
  `Queen's Pomodoro: stopped`. Any other username → `Pomodoro: stopped`.

If you only see this much, the code paths that don't touch Wayland are fine.

## 2. Full test under Hyprland

You need a running Hyprland session. aymm expects:

- `$HYPRLAND_INSTANCE_SIGNATURE` set in the environment (Hyprland sets it
  automatically inside its session).
- The IPC socket at
  `$XDG_RUNTIME_DIR/hypr/$HYPRLAND_INSTANCE_SIGNATURE/.socket.sock`.

### 2.1 Start the daemon

From a terminal *inside* the Hyprland session, **as your normal user**
(not via `sudo` or `su -`, which strip Wayland env vars):

```sh
./bin/aymm                 # foreground; Ctrl+C to stop
./bin/aymm --pomodoro      # one-command Pomodoro launch
```

You should see a small **white cat** appear roughly at the center of your
primary monitor, drawn entirely with Cairo paths. Move your mouse around:
the cat should chase the cursor and stop ~24 px away (the `idle_distance`
setting).

If you have **multiple monitors**, drag the cursor across to a secondary
output: the pet should appear on the monitor where the cursor is and
disappear from the one it just left. This validates the multi-output
overlay.

If you see nothing or get a stderr error, see
[cursor-providers.md](cursor-providers.md) for the diagnostic flow —
the most common cause is running the binary as root (env vars
stripped) or outside a Wayland session.

### 2.2 Drive the daemon from another terminal

Open a second terminal (also as your user, inside the Wayland session) and:

```sh
./bin/aymm status              # human-readable
./bin/aymm toggle              # show/hide pet
./bin/aymm pomodoro start      # focus session
./bin/aymm pomodoro status     # 'focus 24:59' or similar
./bin/aymm pomodoro skip       # jump to break — should fire a notification
./bin/aymm pomodoro stop
./bin/aymm quit                # daemon exits, cat waves goodbye
```

`pomodoro skip` is the fastest way to verify the **notification path**:
it triggers an immediate phase transition, which fires
`notify-send -u normal -a aymm ...`. Make sure you have a notification
daemon running (`mako`, `swaync`, `dunst`, …) and that `notify-send` is
on `$PATH`. The cat also displays an on-screen speech bubble for the
same events.

### 2.3 Validate the Waybar JSON live

While the daemon is running:

```sh
./bin/aymm pomodoro start
./bin/aymm waybar-status        # talks to the daemon now
# {"text":"🐈 24:58","tooltip":"Pomodoro: focus session — 24:58 remaining","class":"focus","percentage":0}
```

Drop `examples/waybar-module.jsonc` into your Waybar config and reload
Waybar (`pkill -SIGUSR2 waybar`). The module should now show the live
timer; left-click toggles Pomodoro, right-click toggles the pet,
middle-click quits the daemon.

### 2.4 Hyprland autostart

Add to `~/.config/hypr/hyprland.conf`:

```
exec-once = aymm
layerrule = blur, aymm
layerrule = ignorezero, aymm
```

Restart Hyprland (`hyprctl dispatch exit` and re-login, or `hyprctl
reload` after the change).

### 2.5 Custom sprite sheet (optional)

The procedural cat is the default. To use your own art, drop a PNG into
`share/aymm/sprites/default/` (next to `sheet.conf`), edit `image=` to
match the file name, and set `sprite_dir=...` in your config (or pass
`--config` pointing at it). See
[sprite-format.md](sprite-format.md) for the manifest reference.

## 3. About the prebuilt tarballs (`dist/`)

`dist/` ships six artifacts: source, NixOS, and Fedora 43 binaries for
both `master` and `feature/queen-bianca-mode`:

| File                                                                     | Use this when…                              |
|--------------------------------------------------------------------------|---------------------------------------------|
| `aymm-0.1.0-master-source.tar.gz`                                        | Building from source (any distro)           |
| `aymm-0.1.0-feature-queen-bianca-mode-source.tar.gz`                     | Same, queen branch                          |
| `aymm-0.1.0-master-nixos-x86_64.tar.gz`                                  | Running on **NixOS**, master persona        |
| `aymm-0.1.0-feature-queen-bianca-mode-nixos-x86_64.tar.gz`               | Running on **NixOS**, queen persona         |
| `aymm-0.1.0-master-fedora43-x86_64.tar.gz`                               | Running on **Fedora 43**, master persona    |
| `aymm-0.1.0-feature-queen-bianca-mode-fedora43-x86_64.tar.gz`            | Running on **Fedora 43**, queen persona     |

**Caveat about the prebuilt binaries**: each was linked against the
runtime libraries shipped by its distro. The NixOS binary depends on
`/nix/store/...` paths and won't run on Fedora; the Fedora binary
depends on Fedora 43's `libwayland`/`libcairo` and will not necessarily
run on older Fedora or other RPM distros.

For other distros, build from the source tarball:

```sh
tar xzf aymm-0.1.0-master-source.tar.gz
cd aymm-0.1.0-master
# Fedora
sudo dnf install cmake gcc-c++ pkgconf-pkg-config wayland-devel \
                  wayland-protocols-devel cairo-devel
# Ubuntu
sudo apt install build-essential cmake pkg-config libwayland-dev \
                  wayland-protocols libwayland-bin libcairo2-dev

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/aymm --version
```

## 4. Compositor compatibility matrix

| Compositor               | Layer-shell overlay | Cursor chase                       |
|--------------------------|---------------------|------------------------------------|
| Hyprland                 | ✅                   | ✅ via Hyprland IPC                 |
| KDE Plasma 6 (KWin)      | ✅                   | ❌ libinput `EVIOCGRAB` exclusivity |
| sway / wlroots generic   | ✅                   | ⚠️ libinput grab, same as KDE       |
| GNOME Shell (Mutter)     | ❌ no wlr-layer-shell | ❌ overlay never opens              |

The **cat renders** anywhere `wlr-layer-shell` is supported, which means
Hyprland, sway, Wayfire, KDE Plasma 6 and most wlroots-based
compositors. **GNOME's Mutter does NOT expose `wlr-layer-shell`** — aymm
fails to bind the global at startup and prints
`compositor missing required globals (compositor/shm/wlr-layer-shell)`.
No cat on GNOME until either Mutter adopts the protocol or aymm gains
an `xdg-toplevel` fallback mode (not planned for v0.1.x).

The chase loop additionally needs a way to read the cursor position —
that works on Hyprland out of the box, and is blocked on every other
compositor by libinput's exclusive grab. On Fedora/Ubuntu the practical
advice is to install Hyprland (`sudo dnf install hyprland` /
`sudo apt install hyprland`) for the chase to actually work.
