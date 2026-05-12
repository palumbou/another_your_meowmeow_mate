#include "aymm/ActionState.hpp"

#include <cmath>

namespace aymm {

namespace {
constexpr auto kDuration = std::chrono::milliseconds(4000);
constexpr double kPi = 3.14159265358979323846;
} // namespace

void ActionState::trigger_next(TimePoint now, double cat_x, double cat_y,
                               bool cat_facing_east) {
    // Pick a random effect, but never the same as the last played one so
    // you don't see the same animation twice in a row.
    static const Effect kPool[4] = {
        Effect::Ball, Effect::Food, Effect::Purr, Effect::Scratch
    };
    std::uniform_int_distribution<int> pick(0, 3);
    Effect chosen = last_played_;
    while (chosen == last_played_) chosen = kPool[pick(rng_)];

    current_     = chosen;
    last_played_ = chosen;
    started_at_  = now;

    // Position the action prop relative to the cat. Bowl and post go in
    // FRONT of the cat (which side that is depends on facing direction).
    const double dx_front = cat_facing_east ? 38.0 : -38.0;
    switch (current_) {
        case Effect::Ball: {
            // Toss the yarn ball in a random direction, 130-180 px away.
            std::uniform_real_distribution<double> ang(0, 2 * kPi);
            std::uniform_real_distribution<double> rad(130, 180);
            const double a = ang(rng_);
            const double r = rad(rng_);
            prop_x = cat_x + std::cos(a) * r;
            prop_y = cat_y + std::sin(a) * r;
            break;
        }
        case Effect::Food:
            // Bowl in front of the cat, slightly below the body line so the
            // cat 'leans in' to eat.
            prop_x = cat_x + dx_front;
            prop_y = cat_y + 18.0;
            break;
        case Effect::Scratch:
            // Scratching post stands in front of the cat.
            prop_x = cat_x + dx_front;
            prop_y = cat_y;
            break;
        case Effect::Purr:
        default:
            prop_x = cat_x;
            prop_y = cat_y;
            break;
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
