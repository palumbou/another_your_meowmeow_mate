cmake_minimum_required(VERSION 3.20)
project(aymm
    VERSION 0.1.3
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
# Pango-cairo gives us proper font fallback + shaping, so emojis in the
# speech bubble render through Noto Color Emoji (or whatever fontconfig
# falls back to) instead of becoming tofu rectangles.
pkg_check_modules(PANGOCAIRO REQUIRED IMPORTED_TARGET pangocairo)

# Vendored layer-shell protocol (from wlr-protocols, MIT-like)
set(LAYER_SHELL_XML "${CMAKE_SOURCE_DIR}/protocols/wlr-layer-shell-unstable-v1.xml")
set(XDG_SHELL_XML   "${WAYLAND_PROTOCOLS_DATADIR}/stable/xdg-shell/xdg-shell.xml")

wayland_protocol_generate(LAYER_SHELL_SRC "${LAYER_SHELL_XML}" wlr-layer-shell-unstable-v1)
wayland_protocol_generate(XDG_SHELL_SRC   "${XDG_SHELL_XML}"   xdg-shell)

add_library(aymm_core STATIC
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
    src/pet/ActionState.cpp
    src/animation/SpriteSheet.cpp
    src/animation/Animation.cpp
    src/animation/ProceduralCat.cpp
    src/animation/SpeechBubble.cpp
    src/pomodoro/PomodoroTimer.cpp
    src/waybar/WaybarStatus.cpp
    src/persona/UserPersona.cpp
    src/cli/Cli.cpp
    src/cli/ControlSocket.cpp
    ${LAYER_SHELL_SRC}
    ${XDG_SHELL_SRC}
)

target_include_directories(aymm_core
    PUBLIC  ${CMAKE_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/protocols
)

target_link_libraries(aymm_core
    PUBLIC
        PkgConfig::WAYLAND_CLIENT
        PkgConfig::CAIRO
        PkgConfig::PANGOCAIRO
        rt
)

# Build-time identifiers — embed git short hash + commit date so users can
# verify they are running the binary they think they are running. Two
# fallbacks because the source can be either a git checkout (development)
# or a release tarball with no .git history (Fedora/Ubuntu build):
#   1. aymm-version-info.cmake ships in release tarballs (preferred)
#   2. live `git` invocation in a checkout
#   3. literal "unknown" otherwise
set(AYMM_GIT_HASH   "unknown")
set(AYMM_BUILD_DATE "unknown")
include(${CMAKE_SOURCE_DIR}/aymm-version-info.cmake OPTIONAL)
if(AYMM_GIT_HASH STREQUAL "unknown")
    execute_process(
        COMMAND git -C ${CMAKE_SOURCE_DIR} rev-parse --short=8 HEAD
        OUTPUT_VARIABLE _AYMM_GH OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET RESULT_VARIABLE _AYMM_GH_RC)
    if(_AYMM_GH_RC EQUAL 0 AND _AYMM_GH)
        set(AYMM_GIT_HASH "${_AYMM_GH}")
    endif()
endif()
if(AYMM_BUILD_DATE STREQUAL "unknown")
    execute_process(
        COMMAND git -C ${CMAKE_SOURCE_DIR} log -1 --format=%cs HEAD
        OUTPUT_VARIABLE _AYMM_GD OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET RESULT_VARIABLE _AYMM_GD_RC)
    if(_AYMM_GD_RC EQUAL 0 AND _AYMM_GD)
        set(AYMM_BUILD_DATE "${_AYMM_GD}")
    endif()
endif()

target_compile_definitions(aymm_core PUBLIC
    AYMM_VERSION="${PROJECT_VERSION}"
    AYMM_GIT_HASH="${AYMM_GIT_HASH}"
    AYMM_BUILD_DATE="${AYMM_BUILD_DATE}"
)

option(AYMM_QUEEN_MODE "Enable Queen persona override (feature branch)" OFF)
if(AYMM_QUEEN_MODE)
    target_compile_definitions(aymm_core PUBLIC AYMM_QUEEN_MODE=1)
endif()

add_executable(aymm src/main.cpp)
target_link_libraries(aymm PRIVATE aymm_core)

install(TARGETS aymm RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY assets/sprites/ DESTINATION ${CMAKE_INSTALL_DATADIR}/aymm/sprites)
install(FILES examples/aymm.conf
        DESTINATION ${CMAKE_INSTALL_DATADIR}/aymm/examples)
install(FILES packaging/desktop/aymm.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications OPTIONAL)
install(FILES packaging/systemd/aymm.service
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/systemd/user OPTIONAL)
