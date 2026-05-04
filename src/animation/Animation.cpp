#include "hyprneko/Animation.hpp"

namespace hyprneko {

const AnimationFrames* AnimationResolver::resolve(PetState s, Direction d) const {
    auto try_name = [&](std::string_view name) { return sheet_.animation(name); };

    switch (s) {
        case PetState::Sleep:           return try_name("sleep");
        case PetState::Wake:            return try_name("wake");
        case PetState::Scratch:         return try_name("scratch");
        case PetState::Idle:            return try_name("idle");
        case PetState::DirectionChange: {
            if (auto* a = try_name("direction_change")) return a;
            return try_name("idle");
        }
        case PetState::Run: {
            std::string name = "run_" + std::string(PetStateMachine::direction_name(d));
            if (auto* a = try_name(name)) return a;
            return try_name("idle");
        }
    }
    return try_name("idle");
}

} // namespace hyprneko
