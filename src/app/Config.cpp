#include "aymm/Config.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace aymm {

namespace {

std::string trim(std::string s) {
    auto issp = [](unsigned char c) { return std::isspace(c); };
    while (!s.empty() && issp(s.front())) s.erase(s.begin());
    while (!s.empty() && issp(s.back()))  s.pop_back();
    return s;
}

bool parse_bool(const std::string& v) {
    if (v == "1" || v == "true" || v == "yes" || v == "on") return true;
    return false;
}

CursorSource parse_cursor_source(const std::string& v) {
    if (v == "hyprland") return CursorSource::Hyprland;
    if (v == "wlroots")  return CursorSource::Wlroots;
    if (v == "evdev")    return CursorSource::Evdev;
    return CursorSource::Null;
}

Behavior parse_behavior(const std::string& v) {
    if (v == "cursor_chase") return Behavior::CursorChase;
    return Behavior::Idle;
}

OverlayLayer parse_layer(const std::string& v) {
    if (v == "background") return OverlayLayer::Background;
    if (v == "bottom")     return OverlayLayer::Bottom;
    if (v == "top")        return OverlayLayer::Top;
    return OverlayLayer::Overlay;
}

} // namespace

std::filesystem::path Config::default_path() {
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        return std::filesystem::path(xdg) / "aymm" / "aymm.conf";
    }
    if (const char* home = std::getenv("HOME"); home && *home) {
        return std::filesystem::path(home) / ".config" / "aymm" / "aymm.conf";
    }
    return {};
}

std::optional<Config> Config::load(const std::filesystem::path& path) {
    std::ifstream in(path);
    if (!in) return std::nullopt;

    Config c;
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string k = trim(line.substr(0, eq));
        const std::string v = trim(line.substr(eq + 1));

        if      (k == "animation_name")       c.animation_name = v;
        else if (k == "sprite_dir")           c.sprite_dir = v;
        else if (k == "behavior")             c.behavior = parse_behavior(v);
        else if (k == "cursor_source")        c.cursor_source = parse_cursor_source(v);
        else if (k == "cursor_poll_hz")       c.cursor_poll_hz = std::atoi(v.c_str());
        else if (k == "movement_speed")       c.movement_speed = std::atoi(v.c_str());
        else if (k == "idle_distance")        c.idle_distance = std::atoi(v.c_str());
        else if (k == "overlay_layer")        c.overlay_layer = parse_layer(v);
        else if (k == "overlay_opacity")      c.overlay_opacity = std::atof(v.c_str());
        else if (k == "monitor")              c.monitor = v;
        else if (k == "surface_size")         c.surface_size = std::atoi(v.c_str());
        else if (k == "enable_waybar_status") c.enable_waybar_status = parse_bool(v);
        else if (k == "enable_pomodoro")      c.enable_pomodoro = parse_bool(v);
        else if (k == "pomodoro_study_minutes")      c.pomodoro_study_minutes = std::atoi(v.c_str());
        else if (k == "pomodoro_break_minutes")      c.pomodoro_break_minutes = std::atoi(v.c_str());
        else if (k == "pomodoro_long_break_minutes") c.pomodoro_long_break_minutes = std::atoi(v.c_str());
        else if (k == "pomodoro_sessions_before_long_break")
            c.pomodoro_sessions_before_long_break = std::atoi(v.c_str());
        else if (k == "pomodoro_auto_start")          c.pomodoro_auto_start = parse_bool(v);
        else if (k == "pomodoro_show_notifications")  c.pomodoro_show_notifications = parse_bool(v);
    }
    return c;
}

Config Config::load_or_default(const std::filesystem::path& path) {
    if (auto opt = load(path)) return *opt;
    return Config{};
}

} // namespace aymm
