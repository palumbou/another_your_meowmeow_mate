# Minimal `nix-shell` entry point for users that don't want to use the flake.
# Usage from the project root:
#
#     nix-shell                         # uses this file by default
#     nix-shell shell.nix               # explicit
#
# Inside the shell:
#
#     cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
#     cmake --build build -j
#     ./build/aymm --version
#
# The flake (`nix build .#aymm`) is still the recommended way on NixOS;
# this file is a convenience for ad-hoc development outside a flake.

{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  name = "aymm-dev";
  packages = with pkgs; [
    cmake
    pkg-config
    wayland
    wayland-protocols
    wayland-scanner
    cairo
    pango
    gcc
    gdb
    valgrind
  ];

  shellHook = ''
    echo "aymm dev shell — try: cmake -S . -B build && cmake --build build -j"
  '';
}
