# Cursor providers — Wayland constraints and provider notes

Wayland deliberately does not expose the global cursor position to arbitrary
clients: every wl_pointer event is scoped to a surface that has the focus.
A "cursor chase" desktop pet therefore needs an out-of-band source for cursor
coordinates. Hyprneko abstracts that source behind `CursorProvider`.

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

## wlroots (reserved)

Generic wlroots compositors do not expose a global cursor command analogous
to `cursorpos`. A reasonable implementation would use a private compositor
plugin or the still-unstable `ext_input_capture_v1` protocol. Not implemented
in v1.

## evdev (reserved)

Reads `/dev/input/event*` directly and accumulates `EV_REL` deltas to maintain
a virtual global cursor. Requires the user to be in the `input` group (or a
udev rule granting access). **Privacy note**: `/dev/input/event*` exposes
keystrokes too — any real implementation MUST whitelist `EV_REL` /
`EV_ABS` / `BTN_*` and never log raw event codes.

## null

Always returns "unknown". Used as a fallback when Hyprland is not detected.
The pet stays put and animates idle/sleep transitions.
