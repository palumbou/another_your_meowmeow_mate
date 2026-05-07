# packaging/

> **Available languages**: [English (current)](README.md) | [Italiano](README.it.md)

Distribution-specific scaffolding to package aymm for end users.

## Layout

| Path                            | Format / target                                                  |
|---------------------------------|------------------------------------------------------------------|
| `desktop/aymm.desktop`          | Freedesktop `.desktop` entry — application launcher integration. |
| `systemd/aymm.service`          | systemd **user** unit; pair with `systemctl --user enable aymm`. |
| `rpm/aymm.spec`                 | RPM spec for Fedora / RHEL / openSUSE-style distros.             |
| `deb/control`, `deb/rules`      | Debian/Ubuntu source-package metadata + rules.                   |

The Nix flake at the project root (`flake.nix`) covers NixOS — install via
`nix profile install .#aymm` or build standalone with `nix build .#aymm`.

## Note on the cat sprite

aymm ships a **custom cat drawn entirely with Cairo paths** as the default
visual — there is no external image asset bundled with the binary. We did
not reuse the classic [oneko](https://github.com/IreneKnapp/oneko) bitmaps
because they don't have a clear license file accompanying them upstream
(the README describes them as "essentially public domain", but the
absence of a real license makes redistribution awkward). The procedural
cat sidesteps the issue entirely: it's our own art, MIT-style permissive
even though the repository itself is CC BY-NC 4.0 (see [LICENSE](../LICENSE)).

If you want a different cat (or a Felix screenmate, or anything else),
drop a PNG sprite sheet into `assets/sprites/default/` and point the
`sprite_dir` config key at it. See
[../docs/sprite-format.md](../docs/sprite-format.md).
