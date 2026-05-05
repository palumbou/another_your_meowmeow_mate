#pragma once

#include "aymm/Pet.hpp"
#include "aymm/PomodoroTimer.hpp"

namespace aymm {

// Modulates the pet's behavior based on the current Pomodoro phase:
// - during Focus: pet stays calm (slower speed, longer idle distance)
// - during Break/LongBreak: pet becomes lively (default speed)
// - while Stopped/Paused: no modulation
class PomodoroBehavior {
public:
    PomodoroBehavior(Pet& pet, PomodoroTimer& timer, int base_speed, int base_idle_distance)
        : pet_(pet), timer_(timer),
          base_speed_(base_speed), base_idle_distance_(base_idle_distance) {}

    void apply();

private:
    Pet& pet_;
    PomodoroTimer& timer_;
    int base_speed_;
    int base_idle_distance_;
};

} // namespace aymm
