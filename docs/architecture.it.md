# Architettura

> **Lingue disponibili**: [English](architecture.md) | [Italiano (corrente)](architecture.it.md)


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
                                              |  aymm CLI <-> daemon     |
                                              +--------------------------+
```

Ogni box è una directory sotto `src/`:

| Modulo                    | Directory             | Compila in        |
|---------------------------|-----------------------|-------------------|
| Wayland overlay           | `src/wayland/`        | layer-shell loop  |
| Cursor providers          | `src/cursor/`         | implementazioni per backend |
| Pet AI                    | `src/pet/`            | FSM + behaviors   |
| Sprite/animation          | `src/animation/`      | loader Cairo PNG  |
| Pomodoro                  | `src/pomodoro/`       | timer puro        |
| Waybar                    | `src/waybar/`         | formatter JSON    |
| Persona                   | `src/persona/`        | saluti/etichette  |
| CLI / control IPC         | `src/cli/`            | parser argv + sock|
| App glue                  | `src/app/`            | run loop          |

L'event loop Wayland (driven da `wl_display_dispatch`) viene multiplexato via
`poll(2)` su:

- l'fd Wayland (frame callback, eventi configure),
- un `timerfd` che ticka a `cursor_poll_hz` (drive cursor poll + pet step),
- l'fd del control socket (subcomandi CLI).

Il rendering avviene quando il compositor manda una frame callback. Il
tick timer richiede un redraw via `OverlaySurface::schedule_redraw()`.
