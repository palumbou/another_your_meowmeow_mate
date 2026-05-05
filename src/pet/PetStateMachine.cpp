#include "aymm/PetStateMachine.hpp"

namespace aymm {

std::string_view PetStateMachine::state_name(PetState s) {
    switch (s) {
        case PetState::Idle:            return "idle";
        case PetState::Wake:            return "wake";
        case PetState::Run:             return "run";
        case PetState::Scratch:         return "scratch";
        case PetState::Sleep:           return "sleep";
        case PetState::DirectionChange: return "direction_change";
    }
    return "?";
}

std::string_view PetStateMachine::direction_name(Direction d) {
    switch (d) {
        case Direction::N:  return "n";
        case Direction::NE: return "ne";
        case Direction::E:  return "e";
        case Direction::SE: return "se";
        case Direction::S:  return "s";
        case Direction::SW: return "sw";
        case Direction::W:  return "w";
        case Direction::NW: return "nw";
        default:            return "none";
    }
}

void PetStateMachine::set_state(PetState s, TimePoint now) {
    if (s == state_) return;
    state_         = s;
    state_entered_ = now;
    frame_         = 0;
    frame_advanced_= now;
}

void PetStateMachine::set_direction(Direction d, TimePoint now) {
    if (d == dir_) return;
    if (state_ == PetState::Run && d != Direction::None) {
        // Brief direction-change wiggle before we commit to the new heading.
        set_state(PetState::DirectionChange, now);
    }
    dir_           = d;
    last_movement_ = now;
}

void PetStateMachine::tick(TimePoint now) {
    using namespace std::chrono;

    // Auto-advance frame every 125ms (matches oneko default tick).
    if (now - frame_advanced_ >= milliseconds(125)) {
        frame_         = (frame_ + 1) % 1024;
        frame_advanced_= now;
    }

    switch (state_) {
        case PetState::DirectionChange:
            if (now - state_entered_ >= direction_change) set_state(PetState::Run, now);
            break;
        case PetState::Wake:
            if (now - state_entered_ >= wake_duration) set_state(PetState::Run, now);
            break;
        case PetState::Idle:
            if (now - last_movement_ >= idle_to_sleep_after) set_state(PetState::Sleep, now);
            break;
        default:
            break;
    }
}

} // namespace aymm
