#include "hyprneko/Pet.hpp"

#include <algorithm>
#include <cmath>

namespace hyprneko {

namespace {

Direction angle_to_direction(double dx, double dy) {
    if (dx == 0 && dy == 0) return Direction::None;
    // atan2: y axis is inverted in screen coordinates (down = positive y).
    // Map angle into 8-sector compass.
    const double a = std::atan2(-dy, dx); // y-up convention
    const double pi = 3.14159265358979323846;
    // sector index in 0..7 starting from East going CCW.
    int sector = static_cast<int>(std::round(a / (pi / 4.0)));
    while (sector < 0) sector += 8;
    sector %= 8;
    static constexpr Direction kMap[8] = {
        Direction::E,  Direction::NE, Direction::N,  Direction::NW,
        Direction::W,  Direction::SW, Direction::S,  Direction::SE
    };
    return kMap[sector];
}

} // namespace

Direction Pet::direction_to_target() const {
    if (!has_target) return Direction::None;
    return angle_to_direction(target.x - position.x, target.y - position.y);
}

void Pet::step(Clock::time_point now) {
    if (!has_target) {
        fsm.tick(now);
        return;
    }

    const double dx = target.x - position.x;
    const double dy = target.y - position.y;
    const double dist = std::sqrt(dx * dx + dy * dy);

    if (dist <= idle_distance) {
        // Within rest distance — stop moving, settle into idle.
        fsm.set_direction(Direction::None, now);
        if (fsm.state() == PetState::Run || fsm.state() == PetState::DirectionChange)
            fsm.set_state(PetState::Idle, now);
        fsm.tick(now);
        return;
    }

    // Wake from sleep when target appears far again.
    if (fsm.state() == PetState::Sleep || fsm.state() == PetState::Idle) {
        fsm.set_state(PetState::Wake, now);
    }

    const Direction dir = angle_to_direction(dx, dy);
    fsm.set_direction(dir, now);

    if (fsm.state() == PetState::Run) {
        const double step = std::min(static_cast<double>(speed), dist);
        position.x += dx / dist * step;
        position.y += dy / dist * step;
    }

    fsm.tick(now);
}

} // namespace hyprneko
