{ lib
, stdenv
, cmake
, pkg-config
, wayland
, wayland-protocols
, wayland-scanner
, cairo
, queenMode ? false
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "aymm";
  version = "0.1.0";

  src = lib.cleanSource ../.;

  nativeBuildInputs = [ cmake pkg-config wayland-scanner ];
  buildInputs = [ wayland wayland-protocols cairo ];

  cmakeFlags = lib.optional queenMode "-DAYMM_QUEEN_MODE=ON";

  meta = with lib; {
    description = "Wayland-native desktop pet that chases your cursor under Hyprland";
    license = licenses.mit;
    platforms = platforms.linux;
    mainProgram = "aymm";
  };
})
