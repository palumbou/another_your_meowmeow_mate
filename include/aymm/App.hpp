#pragma once

#include "aymm/ActionState.hpp"
#include "aymm/Animation.hpp"
#include "aymm/Config.hpp"
#include "aymm/ControlSocket.hpp"
#include "aymm/CursorChaseBehavior.hpp"
#include "aymm/CursorProvider.hpp"
#include "aymm/EvdevCursorProvider.hpp"
#include "aymm/HyprlandCursorProvider.hpp"
#include "aymm/Notifier.hpp"
#include "aymm/NullCursorProvider.hpp"
#include "aymm/OverlaySurface.hpp"
#include "aymm/Pet.hpp"
#include "aymm/PomodoroBehavior.hpp"
#include "aymm/PomodoroTimer.hpp"
#include "aymm/SpeechBubble.hpp"
#include "aymm/SpriteSheet.hpp"

#include <memory>

namespace aymm {

struct AppOptions {
    bool autostart_pomodoro = false;  // --pomodoro CLI flag
};

class App {
public:
    App(Config cfg, AppOptions opts = {});
    int run();

    // Handler for control-socket requests. Returns reply line.
    std::string handle_request(const std::string& req);

private:
    void on_tick();
    void draw(cairo_t* cr, int w, int h, int origin_x, int origin_y);
    void draw_action(cairo_t* cr);
    void on_pomodoro_phase_changed(PomodoroPhase from, PomodoroPhase to);
    Vec2 focus_corner_position() const;
    void say(std::string_view text,
             std::chrono::milliseconds duration = std::chrono::milliseconds(4500),
             SpeechBubble::Style style = SpeechBubble::Style::Speech);

    Config                  cfg_;
    AppOptions              opts_;
    PomodoroTimer           pomodoro_;
    PomodoroPhase           last_pomodoro_phase_ = PomodoroPhase::Stopped;
    PomodoroBehavior        pomodoro_behavior_;
    Pet                     pet_;
    std::unique_ptr<CursorProvider> cursor_;
    EvdevCursorProvider*    evdev_ = nullptr;        // non-owning, when active
    std::unique_ptr<CursorChaseBehavior> chase_;
    std::unique_ptr<SpriteSheet> sheet_;
    std::unique_ptr<AnimationResolver> animations_;
    OverlaySurface          overlay_;
    ControlSocket           control_;
    Notifier                notifier_;
    SpeechBubble            bubble_;
    ActionState             action_;
    int                     last_region_gx_ = -10000, last_region_gy_ = -10000;
    bool                    pet_visible_ = true;
};

} // namespace aymm
