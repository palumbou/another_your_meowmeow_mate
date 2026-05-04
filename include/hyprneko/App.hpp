#pragma once

#include "hyprneko/Animation.hpp"
#include "hyprneko/Config.hpp"
#include "hyprneko/ControlSocket.hpp"
#include "hyprneko/CursorChaseBehavior.hpp"
#include "hyprneko/CursorProvider.hpp"
#include "hyprneko/HyprlandCursorProvider.hpp"
#include "hyprneko/Notifier.hpp"
#include "hyprneko/NullCursorProvider.hpp"
#include "hyprneko/OverlaySurface.hpp"
#include "hyprneko/Pet.hpp"
#include "hyprneko/PomodoroBehavior.hpp"
#include "hyprneko/PomodoroTimer.hpp"
#include "hyprneko/SpriteSheet.hpp"

#include <memory>

namespace hyprneko {

class App {
public:
    explicit App(Config cfg);
    int run();

    // Handler for control-socket requests. Returns reply line.
    std::string handle_request(const std::string& req);

private:
    void on_tick();
    void draw(cairo_t* cr, int w, int h, int origin_x, int origin_y);
    void on_pomodoro_phase_changed(PomodoroPhase from, PomodoroPhase to);

    Config                  cfg_;
    PomodoroTimer           pomodoro_;
    PomodoroPhase           last_pomodoro_phase_ = PomodoroPhase::Stopped;
    PomodoroBehavior        pomodoro_behavior_;
    Pet                     pet_;
    std::unique_ptr<CursorProvider> cursor_;
    std::unique_ptr<CursorChaseBehavior> chase_;
    std::unique_ptr<SpriteSheet> sheet_;
    std::unique_ptr<AnimationResolver> animations_;
    OverlaySurface          overlay_;
    ControlSocket           control_;
    Notifier                notifier_;
    bool                    pet_visible_ = true;
};

} // namespace hyprneko
