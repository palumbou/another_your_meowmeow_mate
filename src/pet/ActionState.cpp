#include "aymm/ActionState.hpp"

#include <cmath>

namespace aymm {

namespace {
constexpr auto kDuration   = std::chrono::milliseconds(3500);
constexpr auto kCycleReset = std::chrono::seconds(10);
constexpr double kPi = 3.14159265358979323846;
} // namespace

void ActionState::trigger_next(TimePoint now, double cat_x, double cat_y) {
    // Reset the cycle if it's been a while since the last click. Avoids
    // landing on a weird state after the user comes back to the cat.
    if (now - last_click_ > kCycleReset) next_index_ = 0;
    last_click_ = now;

    static const Effect kSequence[3] = { Effect::Ball, Effect::Food, Effect::Purr };
    current_    = kSequence[next_index_ % 3];
    next_index_ = (next_index_ + 1) % 3;
    started_at_ = now;

    if (current_ == Effect::Ball) {
        // Toss the ball in a random direction, ~120-180 px from the cat.
        std::uniform_real_distribution<double> ang(0, 2 * kPi);
        std::uniform_real_distribution<double> rad(120, 180);
        const double a = ang(rng_);
        const double r = rad(rng_);
        ball_x = cat_x + std::cos(a) * r;
        ball_y = cat_y + std::sin(a) * r;
    }
}

void ActionState::update(TimePoint now) {
    if (current_ == Effect::None) return;
    if (now - started_at_ >= kDuration) current_ = Effect::None;
}

double ActionState::progress(TimePoint now) const {
    if (current_ == Effect::None) return 0.0;
    using D = std::chrono::duration<double>;
    const double elapsed = std::chrono::duration_cast<D>(now - started_at_).count();
    const double total   = std::chrono::duration_cast<D>(kDuration).count();
    if (total <= 0) return 0.0;
    return std::min(1.0, elapsed / total);
}

} // namespace aymm
