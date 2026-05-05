#include "aymm/CursorChaseBehavior.hpp"

namespace aymm {

void CursorChaseBehavior::update(Pet::Clock::time_point now) {
    if (auto pos = provider_.poll()) {
        pet_.target.x   = pos->x;
        pet_.target.y   = pos->y;
        pet_.has_target = true;
    }
    pet_.step(now);
}

} // namespace aymm
