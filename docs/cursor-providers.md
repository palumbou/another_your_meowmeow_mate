# Cursor providers — Wayland constraints and provider notes

> **Available languages**: [English (current)](cursor-providers.md) | [Italiano](cursor-providers.it.md)


Wayland deliberately does not expose the global cursor position to arbitrary
clients: every wl_pointer event is scoped to a surface that has the focus.
A "cursor chase" desktop pet therefore needs an out-of-band source for cursor
coordinates. Another Your MeowMeow Mate abstracts that source behind `CursorProvider`.

## hyprland (default)

Connects to Hyprland's IPC socket1 at
`$XDG_RUNTIME_DIR/hypr/$HYPRLAND_INSTANCE_SIGNATURE/.socket.sock` and issues
the `cursorpos` command — the same command `hyprctl cursorpos` issues. Each
poll opens a fresh connection (Hyprland's socket1 is one-shot); compared to
forking `hyprctl` per poll this avoids ~all the cost (no exec, no Lua/CLI
startup), but it is still strictly polling, capped by `cursor_poll_hz`.

Hyprland's socket2 (event stream) does **not** broadcast cursor movement by
design, so a true push model is not currently available without
compositor-side changes.

## evdev (fallback for non-Hyprland)

Reads `/dev/input/event*` directly and accumulates `EV_REL` deltas to maintain
a virtual global cursor clamped to the compositor bounds. Requires the user
to be in the `input` group (or a udev rule granting access).

**Important caveat**: most modern Wayland compositors (KDE Plasma 6, sway,
Wayfire, etc.) use libinput, which calls `EVIOCGRAB` on input devices to take
exclusive ownership. When that happens, aymm's `read()` calls return zero
events even though the file descriptors opened successfully. The cat will
render correctly but won't chase the cursor. There is no public Wayland or
KDE/GNOME API to read the global cursor position from a regular client, so
this is a stack-level limitation we can't work around — Hyprland is the
exception because it ships its own IPC.

**Privacy**: aymm's evdev provider whitelists only `EV_REL` X/Y events.
Keyboard events, button events, and absolute touch events are ignored. We
never log raw event codes.

## wlroots (reserved)

Reserved for a future implementation that uses an unstable Wayland protocol
(e.g. `ext_input_capture_v1` once it stabilizes). Not implemented today.

## null

Always returns "unknown". Used when no other provider can be brought up.
The pet stays put and animates idle/sleep transitions.

## Auto-detection

`cursor_source=auto` (the default) picks the best available provider:

1. If `HYPRLAND_INSTANCE_SIGNATURE` is set and the IPC socket is reachable,
   use **hyprland**.
2. Otherwise try **evdev**. If KDE Plasma is detected, the user is warned
   that the chase loop is unlikely to work because of the libinput grab.
3. If no provider is available, fall back to **null**.

Pass `cursor_source=hyprland` or `cursor_source=evdev` to bypass auto-detect.
