#include "hyprneko/WaybarStatus.hpp"
#include "hyprneko/UserPersona.hpp"

#include <cstdio>
#include <sstream>
#include <string>

namespace hyprneko {

namespace {

std::string format_mmss(std::chrono::seconds s) {
    const auto total = s.count() < 0 ? 0 : s.count();
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%02ld:%02ld",
                  static_cast<long>(total / 60), static_cast<long>(total % 60));
    return buf;
}

// JSON string escape — minimal but safe for all the strings we control here.
std::string json_escape(std::string_view in) {
    std::string out;
    out.reserve(in.size() + 2);
    for (char c : in) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char esc[8];
                    std::snprintf(esc, sizeof(esc), "\\u%04x", c);
                    out += esc;
                } else {
                    out += c;
                }
        }
    }
    return out;
}

} // namespace

WaybarPayload waybar_payload(const PomodoroTimer& timer, bool pet_running) {
    using namespace std::chrono;
    WaybarPayload p;
    const auto now      = PomodoroTimer::Clock::now();
    const auto phase    = timer.phase();
    const auto active   = (phase == PomodoroPhase::Paused) ? timer.paused_phase() : phase;
    const auto& persona = UserPersona::active();
    const auto rem      = timer.remaining(now);

    const std::string icon = pet_running ? "\xF0\x9F\x90\x88" /* 🐈 */ : "\xF0\x9F\x92\xA4" /* 💤 */;

    if (phase == PomodoroPhase::Stopped) {
        p.text       = pet_running ? icon + " idle" : icon;
        p.tooltip    = std::string(persona.tooltip_prefix()) + ": stopped";
        p.css_class  = "stopped";
        p.percentage = 0;
        return p;
    }

    p.text       = icon + " " + format_mmss(rem);
    p.percentage = timer.progress_percent(now);

    std::string phase_label;
    switch (active) {
        case PomodoroPhase::Focus:     phase_label = "focus session";   break;
        case PomodoroPhase::Break:     phase_label = "break";           break;
        case PomodoroPhase::LongBreak: phase_label = "long break";      break;
        default:                       phase_label = "session";         break;
    }

    p.tooltip = persona.format_phase_tooltip(
        phase_label, format_mmss(rem),
        /*paused=*/phase == PomodoroPhase::Paused);

    p.css_class = (phase == PomodoroPhase::Paused) ? "paused"
                : std::string(PomodoroTimer::phase_name(active));
    return p;
}

std::string waybar_json(const PomodoroTimer& timer, bool pet_running) {
    const auto p = waybar_payload(timer, pet_running);
    std::ostringstream o;
    o << "{\"text\":\""       << json_escape(p.text)
      << "\",\"tooltip\":\""  << json_escape(p.tooltip)
      << "\",\"class\":\""    << json_escape(p.css_class)
      << "\",\"percentage\":" << p.percentage << "}";
    return o.str();
}

} // namespace hyprneko
