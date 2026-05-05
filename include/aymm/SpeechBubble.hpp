#pragma once

#include <chrono>
#include <string>
#include <string_view>

typedef struct _cairo cairo_t;

namespace aymm {

// A small comic-style speech bubble that appears next to the cat for a
// configurable duration. The user code calls say(...) to push a message;
// draw() renders the bubble while it's still active and returns silently
// when there's nothing to show.
//
// Position is anchored relative to the cat: above-and-to-the-right of the
// pet's center. The bubble's tail points toward (anchor_x, anchor_y).
class SpeechBubble {
public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    void say(std::string_view text,
             std::chrono::milliseconds duration = std::chrono::milliseconds(4500));

    bool active(TimePoint now = Clock::now()) const;

    // Renders the bubble in the current Cairo context. Coordinates are in
    // compositor-global units (caller handles per-output translation).
    void draw(cairo_t* cr, double anchor_x, double anchor_y,
              TimePoint now = Clock::now()) const;

private:
    std::string text_;
    TimePoint   expires_at_{};
    TimePoint   shown_at_{};
};

} // namespace aymm
