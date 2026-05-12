#pragma once

#include <chrono>
#include <string>
#include <string_view>

typedef struct _cairo cairo_t;

namespace aymm {

// A small floating banner above the cat. Two styles:
//   Speech — comic-style with a tail pointing at the cat (cat is talking).
//   Status — plain rounded rectangle, no tail (cat is asleep / on screen
//            announcement, where a "speech" reading would be wrong).
//
// The user code calls say(...) to push a message; draw() renders the
// bubble while it's still active and returns silently when there's
// nothing to show.
class SpeechBubble {
public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    enum class Style { Speech, Status };

    void say(std::string_view text,
             std::chrono::milliseconds duration = std::chrono::milliseconds(4500),
             Style style = Style::Speech);

    bool active(TimePoint now = Clock::now()) const;

    // Renders the bubble in the current Cairo context. Coordinates are in
    // compositor-global units (caller handles per-output translation).
    void draw(cairo_t* cr, double anchor_x, double anchor_y,
              TimePoint now = Clock::now()) const;

private:
    std::string text_;
    Style       style_ = Style::Speech;
    TimePoint   expires_at_{};
    TimePoint   shown_at_{};
};

} // namespace aymm
