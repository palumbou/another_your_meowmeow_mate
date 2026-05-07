# Cursor providers — Vincoli Wayland e note sui provider

> **Lingue disponibili**: [English](cursor-providers.md) | [Italiano (corrente)](cursor-providers.it.md)


Wayland deliberatamente non espone la posizione globale del cursore ai
client generici: ogni evento `wl_pointer` è scoped a una surface che ha
focus. Un desktop pet "cursor chase" ha quindi bisogno di una sorgente
fuori-banda. aymm la astrae dietro `CursorProvider`.

## hyprland (default)

Si connette al socket1 IPC di Hyprland in
`$XDG_RUNTIME_DIR/hypr/$HYPRLAND_INSTANCE_SIGNATURE/.socket.sock` e manda
il comando `cursorpos` — lo stesso comando che `hyprctl cursorpos` usa.
Ogni poll apre una connessione fresca (il socket1 di Hyprland è
one-shot per connessione); rispetto al fork di `hyprctl` per ogni poll
risparmia praticamente tutto il costo (no exec, no startup Lua/CLI), ma
resta polling, capped da `cursor_poll_hz`.

Il socket2 di Hyprland (event stream) **non** trasmette il movimento del
cursore by design, quindi un vero modello push non è disponibile senza
modifiche lato compositor.

## evdev (fallback per i non-Hyprland)

Legge `/dev/input/event*` direttamente e accumula i delta `EV_REL` per
mantenere un cursore globale virtuale clampato ai bounds del compositor.
Richiede che l'utente sia nel gruppo `input` (o una regola udev che
conceda l'accesso).

**Caveat importante**: la maggior parte dei compositor Wayland moderni
(KDE Plasma 6, sway, Wayfire, ecc.) usa libinput, che chiama
`EVIOCGRAB` sui device input per averne possesso esclusivo. Quando
succede, le `read()` di aymm restituiscono zero eventi anche se gli
fd sono stati aperti correttamente. Il gatto verrà disegnato lo stesso
ma non inseguirà il cursore. Non c'è API pubblica Wayland o KDE/GNOME
per leggere la posizione globale del cursore da un client comune,
quindi è un limite stack-level che non possiamo aggirare — Hyprland è
l'eccezione perché ha la sua IPC.

**Privacy**: il provider evdev di aymm whitelist-a solo gli eventi
`EV_REL` X/Y. Eventi tastiera, eventi pulsante, e eventi touch
assoluti vengono ignorati. Non logghiamo mai i codici grezzi.

## wlroots (riservato)

Riservato per un'implementazione futura basata su un protocollo Wayland
unstable (es. `ext_input_capture_v1` quando si stabilizza). Non
implementato oggi.

## null

Restituisce sempre "unknown". Usato quando nessun altro provider può
essere portato su. Il pet sta fermo e fa le transizioni idle/sleep.

## Auto-detection

`cursor_source=auto` (il default) sceglie il miglior provider disponibile:

1. Se `HYPRLAND_INSTANCE_SIGNATURE` è settato e il socket IPC è
   raggiungibile, usa **hyprland**.
2. Altrimenti prova **evdev**. Se viene rilevato KDE Plasma, l'utente
   viene avvisato che la chase loop probabilmente non funzionerà a
   causa del grab di libinput.
3. Se nessun provider è disponibile, fallback a **null**.

Passa `cursor_source=hyprland` o `cursor_source=evdev` per bypassare
l'auto-detect.
