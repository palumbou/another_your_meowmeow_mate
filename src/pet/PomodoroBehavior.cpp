#include "hyprneko/PomodoroBehavior.hpp"

#include <algorithm>

namespace hyprneko {

void PomodoroBehavior::apply() {
    const auto active = (timer_.phase() == PomodoroPhase::Paused)
                      ? timer_.paused_phase() : timer_.phase();
    switch (active) {
        case PomodoroPhase::Focus:
            pet_.speed         = std::max(2, base_speed_ / 2);
            pet_.idle_distance = base_idle_distance_ * 3;
            break;
        case PomodoroPhase::Break:
        case PomodoroPhase::LongBreak:
            pet_.speed         = base_speed_;
            pet_.idle_distance = base_idle_distance_;
            break;
        default:
            pet_.speed         = base_speed_;
            pet_.idle_distance = base_idle_distance_;
            break;
    }
}

} // namespace hyprneko
