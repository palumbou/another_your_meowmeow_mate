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

    enum class Effect { None, Ball, Food, Purr };

    // Each right-click on the cat advances through Ball -> Food -> Purr ->
    // Ball -> ... (and the cycle resets to Ball after ~10 s of inactivity).
    void trigger_next(TimePoint now, double cat_x, double cat_y);

    // Tick: if the effect has expired, clears back to None.
    void update(TimePoint now);

    Effect current() const { return current_; }
    bool   active()  const { return current_ != Effect::None; }
    double progress(TimePoint now) const;

    // Position of the ball (compositor-global), valid when current() == Ball.
    double ball_x = 0;
    double ball_y = 0;

private:
    Effect    current_     = Effect::None;
    TimePoint started_at_{};
    TimePoint last_click_{};
    int       next_index_  = 0;   // 0=Ball, 1=Food, 2=Purr
    std::mt19937 rng_{std::random_device{}()};
};

} // namespace aymm
