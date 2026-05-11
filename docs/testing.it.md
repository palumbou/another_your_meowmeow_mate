# Come testare aymm

> **Lingue disponibili**: [English](testing.md) | [Italiano (corrente)](testing.it.md)


Questa pagina copre tre cose:

1. Smoke test rapidi che non richiedono una sessione Wayland.
2. Il test visivo completo sotto Hyprland (cursor chase + multi-monitor +
   notifiche).
3. Cosa c'è nei tarball pre-buildati e su che sistemi girano.

## 1. Smoke test (no Hyprland)

Funzionano in una shell qualsiasi su qualsiasi Linux:

```sh
./bin/aymm --version
./bin/aymm --help
./bin/aymm waybar-status        # JSON, anche senza daemon
USER=bianca ./bin/aymm waybar-status   # rilevante solo sul branch queen
```

Atteso:
- `--version` stampa `aymm 0.1.0+<hash> (<commit-date>)`
- `waybar-status` stampa JSON valido che termina con `"percentage":0`
- Sul branch queen: `USER=bianca` o `USER=White` → tooltip
  `Queen's Pomodoro: stopped`. Altri username → `Pomodoro: stopped`.

Se fin qui va, i code path che non toccano Wayland sono OK.

## 2. Test completo su Hyprland

Serve una sessione Hyprland in esecuzione. aymm si aspetta:

- `$HYPRLAND_INSTANCE_SIGNATURE` settato nell'environment (Hyprland lo
  setta automaticamente nella sua sessione).
- Il socket IPC in
  `$XDG_RUNTIME_DIR/hypr/$HYPRLAND_INSTANCE_SIGNATURE/.socket.sock`.

### 2.1 Avvia il daemon

Da un terminale **dentro** la sessione Hyprland, **come tuo utente
normale** (non via `sudo` o `su -`, che strippano le env Wayland):

```sh
./bin/aymm                 # foreground; Ctrl+C per fermare
./bin/aymm --pomodoro      # avvio Pomodoro one-shot
```

Dovresti vedere un piccolo **gatto bianco** apparire circa al centro del
monitor primario, disegnato interamente con path Cairo. Muovi il mouse:
il gatto deve inseguire il cursore e fermarsi a ~24 px (`idle_distance`).

Con **più monitor**, trascina il cursore su un output secondario: il pet
deve apparire sul monitor dove c'è il cursore e sparire dall'altro.
Questo valida l'overlay multi-output.

Se non vedi nulla o ottieni un errore su stderr, vedi
[cursor-providers.it.md](cursor-providers.it.md) — la causa più comune è
lanciare il binario come root (env strippate) o fuori da una sessione
Wayland.

### 2.2 Pilota il daemon da un altro terminale

Apri un secondo terminale (anche quello come tuo utente, dentro la
sessione Wayland) e:

```sh
./bin/aymm status              # leggibile
./bin/aymm toggle              # mostra/nascondi pet
./bin/aymm pomodoro start      # focus session
./bin/aymm pomodoro status     # 'focus 24:59' o simile
./bin/aymm pomodoro skip       # salta a break — fa partire una notifica
./bin/aymm pomodoro stop
./bin/aymm quit                # daemon esce, gatto saluta
```

`pomodoro skip` è il modo più rapido di verificare il path delle
**notifiche**: triggera una transizione di fase istantanea, che fa
partire `notify-send -u normal -a aymm ...`. Assicurati di avere un
notification daemon in esecuzione (`mako`, `swaync`, `dunst`, …) e
`notify-send` su `$PATH`. Il gatto mostra anche un fumetto on-screen
per gli stessi eventi.

### 2.3 Valida il JSON Waybar in live

Mentre il daemon è in esecuzione:

```sh
./bin/aymm pomodoro start
./bin/aymm waybar-status        # adesso parla col daemon
# {"text":"🐈 24:58","tooltip":"Pomodoro: focus session — 24:58 remaining","class":"focus","percentage":0}
```

Inserisci `examples/waybar-module.jsonc` nella tua config Waybar e
ricarica Waybar (`pkill -SIGUSR2 waybar`). Il modulo dovrebbe ora
mostrare il timer live; click sx alterna Pomodoro, click dx alterna il
pet, click centrale termina il daemon.

### 2.4 Autostart Hyprland

Aggiungi a `~/.config/hypr/hyprland.conf`:

```
exec-once = aymm
layerrule = blur, aymm
layerrule = ignorezero, aymm
```

Riavvia Hyprland (`hyprctl dispatch exit` e re-login, o `hyprctl
reload` dopo la modifica).

### 2.5 Sprite sheet custom (opzionale)

Il gatto procedurale è il default. Per usare la tua arte, metti un PNG
in `share/aymm/sprites/default/` (accanto a `sheet.conf`), aggiorna
`image=` per matchare il nome file, e setta `sprite_dir=...` nella
config (o passa `--config` che la imposta). Vedi
[sprite-format.it.md](sprite-format.it.md) per la reference del manifest.

## 3. Sui tarball pre-buildati (`dist/`)

`dist/` contiene sei artefatti: source, NixOS e Fedora 43 binari per
entrambi `master` e `feature/queen-bianca-mode`:

| File                                                                     | Quando usarlo                                |
|--------------------------------------------------------------------------|----------------------------------------------|
| `aymm-0.1.0-master-source.tar.gz`                                        | Build da source (qualsiasi distro)           |
| `aymm-0.1.0-feature-queen-bianca-mode-source.tar.gz`                     | Idem, branch queen                           |
| `aymm-0.1.0-master-nixos-x86_64.tar.gz`                                  | Esecuzione su **NixOS**, persona master      |
| `aymm-0.1.0-feature-queen-bianca-mode-nixos-x86_64.tar.gz`               | Esecuzione su **NixOS**, persona queen       |
| `aymm-0.1.0-master-fedora43-x86_64.tar.gz`                               | Esecuzione su **Fedora 43**, persona master  |
| `aymm-0.1.0-feature-queen-bianca-mode-fedora43-x86_64.tar.gz`            | Esecuzione su **Fedora 43**, persona queen   |

**Caveat sui binari pre-buildati**: ogni binario è linkato contro le
runtime library della distro corrispondente. Il binario NixOS dipende
da path `/nix/store/...` e non gira su Fedora; il binario Fedora dipende
dalle `libwayland`/`libcairo` di Fedora 43 e non è garantito che giri
su Fedora più vecchie o altre distro RPM.

Per altre distro, build da source tarball:

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

## 4. Matrice di compatibilità compositor

| Compositor               | Layer-shell overlay     | Cursor chase                       |
|--------------------------|-------------------------|------------------------------------|
| Hyprland                 | ✅                       | ✅ via Hyprland IPC                 |
| KDE Plasma 6 (KWin)      | ✅                       | ❌ libinput `EVIOCGRAB` esclusivo   |
| sway / wlroots generico  | ✅                       | ⚠️ libinput grab, come KDE          |
| GNOME Shell (Mutter)     | ❌ no wlr-layer-shell    | ❌ overlay non si apre              |

Il **gatto viene disegnato** ovunque `wlr-layer-shell` è supportato:
Hyprland, sway, Wayfire, KDE Plasma 6 e la maggior parte dei
compositor wlroots-based. **Mutter di GNOME NON espone
`wlr-layer-shell`** — aymm fallisce il bind del global all'avvio e
stampa `compositor missing required globals (compositor/shm/wlr-layer-shell)`.
Su GNOME niente gatto finché Mutter non adotta il protocollo o aymm
non aggiunge una fallback mode `xdg-toplevel` (non pianificata per v0.1.x).

La chase loop richiede in più di leggere la posizione del cursore —
funziona out-of-the-box su Hyprland, ed è bloccata su tutti gli altri
compositor dal grab esclusivo di libinput. Su Fedora/Ubuntu il
consiglio pratico è installare Hyprland
(`sudo dnf install hyprland` / `sudo apt install hyprland`) per la
chase loop.
