#pragma once

#include "hyprneko/PetStateMachine.hpp"

#include <chrono>

namespace hyprneko {

struct Vec2 {
    double x = 0;
    double y = 0;
};

// The Pet owns position/velocity in compositor global coordinates and a
// state machine. Behaviors push targets and tick the machine.
class Pet {
public:
    using Clock = std::chrono::steady_clock;

    Vec2 position{};
    Vec2 target{};         // current chase target (cursor or rest point)
    bool has_target = false;
    int  speed = 8;        // pixels per tick (movement_speed)
    int  idle_distance = 24;

    PetStateMachine fsm;

    // Move toward target; updates fsm direction/state.
    void step(Clock::time_point now);

    // Direction (8-way) from current pos toward target. NoDirection if at rest.
    Direction direction_to_target() const;
};

} // namespace hyprneko
