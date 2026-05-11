#include "aymm/SpeechBubble.hpp"

#include <cairo/cairo.h>
#include <cmath>

namespace aymm {

namespace {

constexpr double kPi = 3.14159265358979323846;

void rounded_rect(cairo_t* cr, double x, double y, double w, double h, double r) {
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r,     r, -0.5 * kPi, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0,           0.5 * kPi);
    cairo_arc(cr, x + r,     y + h - r, r, 0.5 * kPi,   kPi);
    cairo_arc(cr, x + r,     y + r,     r, kPi,         1.5 * kPi);
    cairo_close_path(cr);
}

} // namespace

void SpeechBubble::say(std::string_view text, std::chrono::milliseconds duration) {
    text_       = text;
    shown_at_   = Clock::now();
    expires_at_ = shown_at_ + duration;
}

bool SpeechBubble::active(TimePoint now) const {
    return !text_.empty() && now < expires_at_;
}

void SpeechBubble::draw(cairo_t* cr, double anchor_x, double anchor_y, TimePoint now) const {
    if (!active(now)) return;

    // Fade-in (first 200 ms) and fade-out (last 400 ms) for a softer feel.
    const auto life       = expires_at_ - shown_at_;
    const auto since_show = now - shown_at_;
    const auto until_end  = expires_at_ - now;
    double alpha = 1.0;
    if (since_show < std::chrono::milliseconds(200)) {
        alpha = std::chrono::duration<double>(since_show).count() / 0.2;
    } else if (until_end < std::chrono::milliseconds(400)) {
        alpha = std::max(0.0, std::chrono::duration<double>(until_end).count() / 0.4);
    }
    (void)life;

    cairo_save(cr);

    // Measure the text first so we can size the bubble.
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    const double font_size = 14.0;
    cairo_set_font_size(cr, font_size);
    cairo_text_extents_t te;
    cairo_text_extents(cr, text_.c_str(), &te);

    const double pad_x = 12.0, pad_y = 8.0;
    const double bw = te.width  + 2 * pad_x;
    const double bh = font_size + 2 * pad_y;
    // Bubble sits clearly above the cat — high enough that its tail can end
    // *outside* the cat's outline. The cat at scale 4 reaches roughly
    // y = anchor_y - 40 at the ear tips, so the bubble bottom must be at
    // y <= anchor_y - 45 and the tail tip must land at y <= anchor_y - 45.
    const double bx = anchor_x + 32.0;
    const double by = anchor_y - bh - 60.0;

    // Drop shadow
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.20 * alpha);
    rounded_rect(cr, bx + 2, by + 3, bw, bh, 10);
    cairo_fill(cr);

    // Bubble fill
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.96 * alpha);
    rounded_rect(cr, bx, by, bw, bh, 10);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.15, 0.15, 0.20, 0.85 * alpha);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);

    // Tail — small triangle from bubble bottom-left pointing toward the cat
    // but stopping ABOVE the cat's outline (~y = anchor_y - 45 keeps it clear
    // of the ears even when the cat is mid-run).
    const double tail_root_x = bx + 10.0;
    const double tail_root_y = by + bh;
    const double tip_x = anchor_x + 28.0;
    const double tip_y = anchor_y - 45.0;
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.96 * alpha);
    cairo_move_to(cr, tail_root_x,         tail_root_y - 0.5);
    cairo_line_to(cr, tail_root_x + 14.0,  tail_root_y - 0.5);
    cairo_line_to(cr, tip_x,               tip_y);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.15, 0.15, 0.20, 0.85 * alpha);
    cairo_set_line_width(cr, 1.2);
    cairo_stroke(cr);
    // Cover the seam where the tail meets the bubble.
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.96 * alpha);
    cairo_set_line_width(cr, 2.0);
    cairo_move_to(cr, tail_root_x + 1.0, tail_root_y);
    cairo_line_to(cr, tail_root_x + 13.0, tail_root_y);
    cairo_stroke(cr);

    // Text
    cairo_set_source_rgba(cr, 0.10, 0.10, 0.15, alpha);
    cairo_move_to(cr, bx + pad_x - te.x_bearing, by + pad_y - te.y_bearing);
    cairo_show_text(cr, text_.c_str());

    cairo_restore(cr);
}

} // namespace aymm
