#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace aymm {

enum class CursorSource { Auto, Hyprland, Wlroots, Evdev, Null };
enum class Behavior     { CursorChase, Idle };
enum class OverlayLayer { Background, Bottom, Top, Overlay };

// Where the pet parks its basket during Pomodoro focus, to stay out of the
// user's way. `None` keeps the pet wherever it was before focus started.
enum class FocusCorner  { None, BottomRight, BottomLeft, TopRight, TopLeft, Center };

struct Config {
    std::string  animation_name             = "neko";
    std::string  sprite_dir;
    Behavior     behavior                   = Behavior::CursorChase;
    CursorSource cursor_source              = CursorSource::Auto;
    int          cursor_poll_hz             = 30;
    int          movement_speed             = 8;
    int          idle_distance              = 24;
    OverlayLayer overlay_layer              = OverlayLayer::Overlay;
    double       overlay_opacity            = 0.0;
    std::string  monitor                    = "auto";
    int          surface_size               = 256;

    bool         enable_waybar_status       = true;

    bool         enable_pomodoro            = false;
    int          pomodoro_study_minutes     = 25;
    int          pomodoro_break_minutes     = 5;
    int          pomodoro_long_break_minutes= 15;
    int          pomodoro_sessions_before_long_break = 4;
    bool         pomodoro_auto_start        = false;
    bool         pomodoro_show_notifications= true;
    FocusCorner  pomodoro_focus_corner      = FocusCorner::BottomRight;
    int          pomodoro_focus_padding     = 100;   // px from the screen edge

    static Config load_or_default(const std::filesystem::path& path);
    static std::optional<Config> load(const std::filesystem::path& path);
    static std::filesystem::path default_path();
};

} // namespace aymm
