#include "aymm/SpeechBubble.hpp"

#include <algorithm>
#include <cairo/cairo.h>
#include <cmath>
#include <pango/pangocairo.h>

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

void SpeechBubble::say(std::string_view text,
                       std::chrono::milliseconds duration,
                       Style style) {
    text_       = text;
    style_      = style;
    shown_at_   = Clock::now();
    expires_at_ = shown_at_ + duration;
}

bool SpeechBubble::active(TimePoint now) const {
    return !text_.empty() && now < expires_at_;
}

void SpeechBubble::draw(cairo_t* cr, double anchor_x, double anchor_y, TimePoint now) const {
    if (!active(now)) return;

    // Fade-in (first 200 ms) and fade-out (last 400 ms) for a softer feel.
    const auto since_show = now - shown_at_;
    const auto until_end  = expires_at_ - now;
    double alpha = 1.0;
    if (since_show < std::chrono::milliseconds(200)) {
        alpha = std::chrono::duration<double>(since_show).count() / 0.2;
    } else if (until_end < std::chrono::milliseconds(400)) {
        alpha = std::max(0.0, std::chrono::duration<double>(until_end).count() / 0.4);
    }

    cairo_save(cr);

    // Build a Pango layout so emojis get fontconfig-based fallback to a
    // color emoji font (Noto Color Emoji on most systems). Cairo's toy
    // text API doesn't do that — strings like "Focus time, Queen 🐾"
    // would render the paw as a tofu rectangle without Pango.
    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text_.c_str(), -1);
    PangoFontDescription* desc = pango_font_description_from_string("Sans 11");
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    int text_w = 0, text_h = 0;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    const double pad_x = 12.0, pad_y = 7.0;
    const double bw = text_w + 2 * pad_x;
    const double bh = text_h + 2 * pad_y;

    // Position depends on style:
    //   Speech — bubble up-and-to-the-right, with a tail pointing at the cat
    //   Status — centered above the cat, no tail (cat is asleep / banner)
    const bool with_tail = (style_ == Style::Speech);
    const double bx = with_tail ? anchor_x + 32.0 : anchor_x - bw / 2.0;
    const double by = with_tail ? anchor_y - bh - 60.0 : anchor_y - bh - 70.0;

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

    if (with_tail) {
        // Speech tail — small triangle from bubble bottom-left pointing at
        // the cat but stopping ABOVE the cat's outline (~y = anchor_y - 45
        // keeps it clear of the ears even when the cat is mid-run).
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
    }

    // Text via Pango — handles emoji fallback automatically.
    cairo_set_source_rgba(cr, 0.10, 0.10, 0.15, alpha);
    cairo_move_to(cr, bx + pad_x, by + pad_y);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
    cairo_restore(cr);
}

} // namespace aymm
