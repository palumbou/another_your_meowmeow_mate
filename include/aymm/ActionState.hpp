#pragma once

#include <chrono>
#include <random>

namespace aymm {

// A small finite-state machine for the cat's interactive "actions": play
// with a ball, eat from a bowl, purr. Triggered by right-clicking on the
// cat; each right-click cycles to the next effect.
//
// During an active effect:
//   - Ball : a small red ball appears beside the cat; the cat chases it
//            (handled by the App, which sets pet.target to ball_pos).
//   - Food : a brown bowl appears just below the cat; the cat sits to eat.
//   - Purr : the cat sits in place and small pink hearts float up around it.
//
// After `duration` the effect expires and current() returns None.
class ActionState {
public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    // Ball = a yarn ball the cat trots over to. Food = bowl with kibble.
    // Purr = sit and emit hearts. Scratch = scratching post next to cat.
    enum class Effect { None, Ball, Food, Purr, Scratch };

    // Each right-click on the cat picks a random effect (different from the
    // last one, so you never see the same animation twice in a row).
    // cat_facing_east tells us which side of the cat objects should appear
    // on (so the bowl is in front of the cat's head, not behind).
    void trigger_next(TimePoint now, double cat_x, double cat_y, bool cat_facing_east);

    // Tick: if the effect has expired, clears back to None.
    void update(TimePoint now);

    Effect current() const { return current_; }
    bool   active()  const { return current_ != Effect::None; }
    double progress(TimePoint now) const;

    // Compositor-global positions for the action props.
    //   Ball: position of the yarn ball
    //   Food: bowl position (rendered in front of the cat)
    //   Scratch: scratching post position (rendered in front of the cat)
    double prop_x = 0;
    double prop_y = 0;

private:
    Effect    current_     = Effect::None;
    Effect    last_played_ = Effect::None;
    TimePoint started_at_{};
    std::mt19937 rng_{std::random_device{}()};
};

} // namespace aymm
