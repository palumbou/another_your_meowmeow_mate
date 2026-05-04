cmake_minimum_required(VERSION 3.20)
project(hyprneko
    VERSION 0.1.0
    DESCRIPTION "Wayland-native desktop pet that chases your cursor under Hyprland"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)

include(GNUInstallDirs)
include(cmake/WaylandScanner.cmake)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED wayland-protocols>=1.31)
pkg_get_variable(WAYLAND_PROTOCOLS_DATADIR wayland-protocols pkgdatadir)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)

# Vendored layer-shell protocol (from wlr-protocols, MIT-like)
set(LAYER_SHELL_XML "${CMAKE_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml")
set(XDG_SHELL_XML   "${WAYLAND_PROTOCOLS_DATADIR}/stable/xdg-shell/xdg-shell.xml")

wayland_protocol_generate(LAYER_SHELL_SRC "${LAYER_SHELL_XML}" wlr-layer-shell-unstable-v1)
wayland_protocol_generate(XDG_SHELL_SRC   "${XDG_SHELL_XML}"   xdg-shell)

add_library(hyprneko_core STATIC
    src/app/App.cpp
    src/app/Config.cpp
    src/app/Notifier.cpp
    src/wayland/OverlaySurface.cpp
    src/cursor/HyprlandCursorProvider.cpp
    src/cursor/NullCursorProvider.cpp
    src/cursor/EvdevCursorProvider.cpp
    src/pet/Pet.cpp
    src/pet/PetStateMachine.cpp
    src/pet/CursorChaseBehavior.cpp
    src/pet/PomodoroBehavior.cpp
    src/animation/SpriteSheet.cpp
    src/animation/Animation.cpp
    src/pomodoro/PomodoroTimer.cpp
    src/waybar/WaybarStatus.cpp
    src/persona/UserPersona.cpp
    src/cli/Cli.cpp
    src/cli/ControlSocket.cpp
    ${LAYER_SHELL_SRC}
    ${XDG_SHELL_SRC}
)

target_include_directories(hyprneko_core
    PUBLIC  ${CMAKE_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/protocols
)

target_link_libraries(hyprneko_core
    PUBLIC
        PkgConfig::WAYLAND_CLIENT
        PkgConfig::CAIRO
        rt
)

# Build-time identifiers
target_compile_definitions(hyprneko_core PUBLIC
    HYPRNEKO_VERSION="${PROJECT_VERSION}"
)

# This branch defaults the Queen persona override to ON. It activates only at
# runtime when the system username matches a Bianca-variant; other users get
# the neutral persona transparently. Pass -DHYPRNEKO_QUEEN_MODE=OFF to disable.
option(HYPRNEKO_QUEEN_MODE "Enable Queen persona override (feature branch)" ON)
if(HYPRNEKO_QUEEN_MODE)
    target_compile_definitions(hyprneko_core PUBLIC HYPRNEKO_QUEEN_MODE=1)
endif()

add_executable(hyprneko src/main.cpp)
target_link_libraries(hyprneko PRIVATE hyprneko_core)

install(TARGETS hyprneko RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY assets/sprites/ DESTINATION ${CMAKE_INSTALL_DATADIR}/hyprneko/sprites)
install(FILES examples/hyprneko.conf
        DESTINATION ${CMAKE_INSTALL_DATADIR}/hyprneko/examples)
install(FILES packaging/desktop/hyprneko.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications OPTIONAL)
install(FILES packaging/systemd/hyprneko.service
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/user OPTIONAL)
