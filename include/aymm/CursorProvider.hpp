#pragma once

#include <optional>

namespace aymm {

struct CursorPos {
    int x = 0;
    int y = 0;
};

// Abstraction over the source of the global cursor position. Wayland
// deliberately does not expose the global cursor to arbitrary clients, so
// every provider has compositor-specific or input-stack constraints.
//
// Lifetime: providers are polled by the App at config.cursor_poll_hz. They
// must be cheap to call and non-blocking on the typical path.
class CursorProvider {
public:
    virtual ~CursorProvider() = default;

    // Returns std::nullopt if the position is currently unknown (e.g. socket
    // unreachable). The caller should treat that as "do not move this tick".
    virtual std::optional<CursorPos> poll() = 0;

    // Human-readable name for diagnostics / logging.
    virtual const char* name() const = 0;
};

} // namespace aymm
