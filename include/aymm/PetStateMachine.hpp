#pragma once

#include <chrono>
#include <string_view>

namespace aymm {

// 8-way direction set, matching oneko's NW/N/NE/W/E/SW/S/SE compass.
enum class Direction : int {
    None = -1,
    N = 0, NE, E, SE, S, SW, W, NW
};

// State set inspired by oneko.
enum class PetState {
    Idle,
    Wake,
    Run,
    Scratch,        // wall-scratch when stuck at an edge
    Sleep,
    DirectionChange // brief animation when changing direction
};

class PetStateMachine {
public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    PetState  state()     const { return state_; }
    Direction direction() const { return dir_;   }
    int       frame()     const { return frame_; }

    // Default thresholds tuned to oneko's historical defaults.
    std::chrono::milliseconds idle_to_sleep_after = std::chrono::seconds(10);
    std::chrono::milliseconds wake_duration       = std::chrono::milliseconds(400);
    std::chrono::milliseconds direction_change    = std::chrono::milliseconds(120);

    void set_direction(Direction d, TimePoint now);
    void set_state(PetState s, TimePoint now);
    void tick(TimePoint now);

    static std::string_view state_name(PetState s);
    static std::string_view direction_name(Direction d);

private:
    PetState  state_ = PetState::Idle;
    Direction dir_   = Direction::None;
    TimePoint state_entered_{};
    TimePoint last_movement_{};
    int       frame_ = 0;
    TimePoint frame_advanced_{};
};

} // namespace aymm
