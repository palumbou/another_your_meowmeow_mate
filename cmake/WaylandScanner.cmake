# Helper to invoke wayland-scanner at build time.
# Generates <name>-client-protocol.h and <name>-protocol.c.
find_program(WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner REQUIRED)

function(wayland_protocol_generate OUT_VAR XML_PATH BASENAME)
    set(OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/protocols")
    file(MAKE_DIRECTORY "${OUT_DIR}")
    set(HEADER "${OUT_DIR}/${BASENAME}-client-protocol.h")
    set(SOURCE "${OUT_DIR}/${BASENAME}-protocol.c")

    add_custom_command(
        OUTPUT "${HEADER}"
        COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" client-header "${XML_PATH}" "${HEADER}"
        DEPENDS "${XML_PATH}"
        VERBATIM)

    add_custom_command(
        OUTPUT "${SOURCE}"
        COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" private-code "${XML_PATH}" "${SOURCE}"
        DEPENDS "${XML_PATH}"
        VERBATIM)

    set(${OUT_VAR} "${HEADER}" "${SOURCE}" PARENT_SCOPE)
endfunction()
