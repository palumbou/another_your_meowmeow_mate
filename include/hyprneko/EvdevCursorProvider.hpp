#pragma once

#include "hyprneko/CursorProvider.hpp"

namespace hyprneko {

// Stub. A real implementation would open /dev/input/event* devices (requires
// the user to be in the `input` group, or a udev rule) and accumulate
// EV_REL deltas to maintain a virtual global cursor position.
//
// Privacy note: /dev/input devices receive ALL input including keystrokes.
// A real implementation MUST whitelist EV_REL/EV_ABS/BTN_* and never log key
// codes. See docs/cursor-providers.md for the full design.
class EvdevCursorProvider final : public CursorProvider {
public:
    std::optional<CursorPos> poll() override { return std::nullopt; }
    const char* name() const override { return "evdev (stub)"; }
};

} // namespace hyprneko
