#pragma once

#include "hyprneko/PomodoroTimer.hpp"

#include <string>

namespace hyprneko {

struct WaybarPayload {
    std::string text;
    std::string tooltip;
    std::string css_class;
    int         percentage = 0;
};

// Build a Waybar custom-module JSON payload. Emits the format documented in
// docs/waybar-module.jsonc — text, tooltip, class, percentage.
std::string waybar_json(const PomodoroTimer& timer, bool pet_running);

WaybarPayload waybar_payload(const PomodoroTimer& timer, bool pet_running);

} // namespace hyprneko
