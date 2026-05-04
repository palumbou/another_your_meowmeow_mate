# Architecture

```
            +-----------+   +------------------+   +---------------+
  Hyprland  |  Cursor   |   |       Pet        |   |  Pomodoro     |
   socket   | Provider  +-->|  StateMachine    |<--+   Timer       |
            +-----+-----+   +---------+--------+   +---------------+
                  ^                   |                     ^
                  |   (poll @ N Hz)   v                     |
            +-----+-------+   +-------+-------+   +---------+--------+
            | CursorChase |   |  Animation    |   |  Waybar JSON     |
            |  Behavior   |   |  Resolver     |   |   builder        |
            +-------------+   +-------+-------+   +------------------+
                                      |
                                      v
                              +---------------+    +-------------------+
                              | Sprite Sheet  |    |  Overlay surface  |
                              |   (Cairo PNG) +--->| wlr-layer-shell + |
                              +---------------+    |    wl_shm + Cairo |
                                                   +-------------------+
                                                            ^
                                                            |
                                              +--------------------------+
                                              |  Control socket (UNIX)   |
                                              |  hyprneko CLI <-> daemon |
                                              +--------------------------+
```

Each box maps to one directory under `src/`:

| Module                | Directory             | Compiles to       |
|-----------------------|-----------------------|-------------------|
| Wayland overlay       | `src/wayland/`        | layer-shell loop  |
| Cursor providers      | `src/cursor/`         | per-source impls  |
| Pet AI                | `src/pet/`            | FSM + behaviors   |
| Sprite/animation      | `src/animation/`      | Cairo PNG loader  |
| Pomodoro              | `src/pomodoro/`       | pure timer        |
| Waybar                | `src/waybar/`         | JSON formatter    |
| Persona               | `src/persona/`        | greetings/labels  |
| CLI / control IPC     | `src/cli/`            | argv parser + sock|
| App glue              | `src/app/`            | run loop          |

The Wayland event loop (driven by `wl_display_dispatch`) is multiplexed via
`poll(2)` against:

- the Wayland fd (frame callbacks, configure events),
- a `timerfd` ticking at `cursor_poll_hz` (drives cursor poll + pet step),
- the control socket fd (CLI subcommands).

Rendering happens whenever the compositor sends a frame callback. The tick
timer requests a redraw via `OverlaySurface::schedule_redraw()`.
