#pragma once

#include "aymm/CursorProvider.hpp"

#include <string>
#include <vector>

namespace aymm {

// Reads relative pointer movement (EV_REL X/Y) from /dev/input/event* and
// maintains a virtual cursor position clamped to the compositor bounds the
// caller installs via set_bounds(). Works on any compositor that exposes
// such devices — used as the fallback when we're not on Hyprland (e.g.
// KDE Plasma 6, sway without IPC, etc).
//
// Permission: /dev/input nodes are typically owned by root:input. The
// caller account must be in the `input` group, OR a udev rule must grant
// access. open() failures are silent — the provider just degrades to
// "position unknown" until something is readable.
//
// Privacy: we whitelist the EV_REL events we care about and never log key
// codes. Touch / tablet events are ignored.
class EvdevCursorProvider final : public CursorProvider {
public:
    EvdevCursorProvider();
    ~EvdevCursorProvider() override;

    EvdevCursorProvider(const EvdevCursorProvider&)            = delete;
    EvdevCursorProvider& operator=(const EvdevCursorProvider&) = delete;

    std::optional<CursorPos> poll() override;
    const char* name() const override { return "evdev"; }

    bool ready() const { return !fds_.empty(); }
    int  device_count() const { return static_cast<int>(fds_.size()); }

    // Defines the rectangle the virtual cursor is clamped to. Should be set
    // to the compositor bounds (sum of all wl_outputs) once those are known.
    void set_bounds(int x, int y, int w, int h);

    // Initial cursor position. Defaults to the center of the bounds.
    void set_position(int x, int y);

private:
    std::vector<int> fds_;
    int  bx_ = 0, by_ = 0, bw_ = 1920, bh_ = 1080;
    double cx_ = 960, cy_ = 540;
};

} // namespace aymm
