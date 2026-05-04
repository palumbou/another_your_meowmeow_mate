{
  description = "Hyprneko — Wayland-native desktop pet that chases your cursor under Hyprland";

  inputs = {
    nixpkgs.url      = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url  = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        callPkg = queenMode: pkgs.callPackage ./nix/default.nix { inherit queenMode; };
      in {
        packages = {
          default   = callPkg false;
          hyprneko  = callPkg false;
          # Build with the Queen persona override compiled in. Kept here for
          # convenience; the upstream branch is feature/queen-bianca-mode.
          hyprneko-queen = callPkg true;
        };

        apps.default = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/hyprneko";
        };

        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            cmake pkg-config wayland wayland-protocols wayland-scanner cairo
            gcc gdb valgrind
          ];
        };
      });
}
