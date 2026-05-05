# How to test aymm

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
- `--version` prints `aymm 0.1.0`
- `waybar-status` prints valid JSON ending in `"percentage":0`
- On the queen branch: `USER=bianca` or `USER=White` → tooltip says
  `Queen's Pomodoro: stopped`. Any other username → `Pomodoro: stopped`.

If you only see this much, the code paths that don't touch Wayland are fine.

## 2. Full test under Hyprland

You need a running Hyprland session. Another Your MeowMeow Mate expects:

- `$HYPRLAND_INSTANCE_SIGNATURE` set in the environment (Hyprland sets it
  automatically inside its session).
- The IPC socket at
  `$XDG_RUNTIME_DIR/hypr/$HYPRLAND_INSTANCE_SIGNATURE/.socket.sock`.

### 2.1 Start the daemon

From a terminal *inside* the Hyprland session:

```sh
./bin/aymm          # foreground; Ctrl+C to stop
```

You should immediately see a small **orange circle** appear roughly at
the center of your primary monitor (this is the placeholder pet — no
sprite ships with the repo). Move your mouse around: the circle should
chase the cursor and stop ~24 px away (the `idle_distance` setting).

If you have **multiple monitors**, drag the cursor across to a secondary
output: the pet should appear on the monitor where the cursor is and
disappear from the one it just left. This validates the multi-output
overlay.

If you see nothing:

- Check stderr. The most common message is
  `Hyprland cursor socket not detected; falling back to null provider.`
  That means `HYPRLAND_INSTANCE_SIGNATURE` isn't set or the IPC socket
  doesn't exist — make sure you launched the binary from inside a real
  Hyprland session, not from another compositor/TTY.
- Check `compositor missing required globals` — your compositor doesn't
  expose `wlr-layer-shell`. KDE Plasma and GNOME Shell don't; Hyprland,
  Sway, Wayfire and most wlroots-based compositors do.

### 2.2 Drive the daemon from another terminal

Open a second terminal and:

```sh
./bin/aymm status              # human-readable
./bin/aymm toggle              # show/hide pet
./bin/aymm pomodoro start      # focus session
./bin/aymm pomodoro status     # 'focus 24:59' or similar
./bin/aymm pomodoro skip       # jump to break — should fire a notification
./bin/aymm pomodoro stop
./bin/aymm quit                # daemon exits
```

`pomodoro skip` is the fastest way to verify the **notification path**:
it triggers an immediate phase transition, which fires
`notify-send -u normal -a aymm ...`. Make sure you have a
notification daemon running (`mako`, `swaync`, `dunst`, …) and that
`notify-send` is on `$PATH`.

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
exec-once = /path/to/aymm
layerrule = blur, aymm
layerrule = ignorezero, aymm
```

Restart Hyprland (`hyprctl dispatch exit` and re-login, or `hyprctl
reload` after the change).

## 3. About the prebuilt tarballs (`dist/`)

`dist/` ships four artifacts:

| File                                                    | Use this when…                          |
|---------------------------------------------------------|-----------------------------------------|
| `aymm-0.1.0-master-source.tar.gz`                   | Building from source (any distro)       |
| `aymm-0.1.0-queen-bianca-source.tar.gz`             | Same, queen branch                      |
| `aymm-0.1.0-master-nixos-x86_64.tar.gz`             | Running on **NixOS**, master persona    |
| `aymm-0.1.0-feature-queen-bianca-mode-nixos-x86_64.tar.gz` | Running on **NixOS**, queen persona |

**Important caveat about the prebuilt binaries**: they were compiled
inside a `nix-shell` and are dynamically linked against `wayland-client`
and `cairo` libraries that live under `/nix/store/...`. They will run
on **NixOS** (where those store paths exist) and likely on any system
with `nix` installed and the required closures fetched, but they will
**not** run as-is on Fedora or Ubuntu.

For Fedora / Ubuntu, build from the source tarball:

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

The build system has been tested only on NixOS; the Fedora/Ubuntu
package lists above match the current upstream package names but the
end-to-end pipeline on those distros has not been verified by me.

## 4. What is *not* tested in v0.1

- Sprite rendering on a real screen — the placeholder circle is what
  proves the chase loop. To validate sprite rendering, drop a `neko.png`
  into `share/aymm/sprites/neko/` and adjust `frame_w`/`frame_h` in
  `sheet.conf` if needed; then set `sprite_dir=...` in your config.
- The `wlroots` and `evdev` cursor providers — both are documented stubs
  in [docs/cursor-providers.md](cursor-providers.md).
- KDE/GNOME — those compositors don't expose `wlr-layer-shell`, so
  aymm cannot create its overlay there.
