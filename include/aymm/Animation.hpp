#pragma once

#include "aymm/PetStateMachine.hpp"
#include "aymm/SpriteSheet.hpp"

#include <string>
#include <string_view>

namespace aymm {

// Resolves a (state, direction) pair to an animation name in the sprite
// sheet. The naming convention is:
//   idle, sleep, wake, scratch
//   run_n, run_ne, run_e, run_se, run_s, run_sw, run_w, run_nw
// If a direction-specific animation is not present, falls back gracefully.
class AnimationResolver {
public:
    explicit AnimationResolver(const SpriteSheet& sheet) : sheet_(sheet) {}

    const AnimationFrames* resolve(PetState s, Direction d) const;

private:
    const SpriteSheet& sheet_;
};

} // namespace aymm
