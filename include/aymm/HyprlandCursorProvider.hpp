#pragma once

#include "aymm/CursorProvider.hpp"

#include <optional>
#include <string>

namespace aymm {

// Polls Hyprland's IPC socket1 for cursor position.
//
// We open a fresh AF_UNIX connection per poll and send the "cursorpos" command
// (the same command `hyprctl cursorpos` issues). Hyprland's socket1 protocol
// is one-shot per connection, so this is a connect/write/read/close cycle —
// vastly cheaper than fork+exec of the `hyprctl` binary, but still polling.
//
// Note: socket2 (event stream) does not broadcast cursor movement by design,
// so a true push model is not available without compositor-side changes.
class HyprlandCursorProvider final : public CursorProvider {
public:
    HyprlandCursorProvider();

    std::optional<CursorPos> poll() override;
    const char* name() const override { return "hyprland"; }

    // Returns the resolved socket path, or empty string if not detected.
    const std::string& socket_path() const { return socket_path_; }
    bool ready() const { return !socket_path_.empty(); }

private:
    static std::string detect_socket_path();
    std::string socket_path_;
};

} // namespace aymm
