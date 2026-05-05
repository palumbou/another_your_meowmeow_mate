#include "hyprneko/ProceduralCat.hpp"

#include <cairo/cairo.h>
#include <cmath>

namespace hyprneko {

namespace {

constexpr double kPi = 3.14159265358979323846;

// Default art faces EAST (head on +x). Mirror for any direction whose
// horizontal component is negative — that is W, NW, SW. Pure N/S keep the
// current heading; we don't bother flipping head-up/down for those.
bool faces_west(Direction d) {
    return d == Direction::W || d == Direction::NW || d == Direction::SW;
}

void path_ellipse(cairo_t* cr, double cx, double cy, double rx, double ry) {
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    cairo_scale(cr, rx, ry);
    cairo_arc(cr, 0, 0, 1, 0, 2 * kPi);
    cairo_restore(cr);
}

// All routines assume an east-facing cat: head at +x ~+8, body centered,
// tail at -x ~-9. The caller does the horizontal flip for west.

void fill_body_and_head(cairo_t* cr) {
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    path_ellipse(cr, 0, 4, 9, 6);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.30, 0.30, 0.30, 0.85);
    cairo_set_line_width(cr, 0.6);
    cairo_stroke(cr);

    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    cairo_arc(cr, 8, -2, 5.5, 0, 2 * kPi);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.30, 0.30, 0.30, 0.85);
    cairo_stroke(cr);
}

void draw_ears(cairo_t* cr) {
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    // Right ear (front)
    cairo_move_to(cr, 12, -5);
    cairo_line_to(cr, 11, -10);
    cairo_line_to(cr, 8,  -4);
    cairo_close_path(cr);
    cairo_fill(cr);
    // Left ear (back)
    cairo_move_to(cr, 8,  -7);
    cairo_line_to(cr, 5, -10);
    cairo_line_to(cr, 4,  -4);
    cairo_close_path(cr);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, 0.95, 0.65, 0.70, 0.9);
    cairo_move_to(cr, 11, -6);
    cairo_line_to(cr, 10, -8);
    cairo_line_to(cr,  9, -5);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_move_to(cr, 7, -6);
    cairo_line_to(cr, 6, -8);
    cairo_line_to(cr, 5, -5);
    cairo_close_path(cr);
    cairo_fill(cr);
}

void draw_face(cairo_t* cr, PetState s) {
    // Eyes — head is at +x, so eyes are on the +x side.
    cairo_set_source_rgba(cr, 0.10, 0.10, 0.10, 1.0);
    if (s == PetState::Sleep) {
        cairo_set_line_width(cr, 0.8);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_move_to(cr, 5.5, -2.5); cairo_line_to(cr, 7.5, -2.5);
        cairo_stroke(cr);
        cairo_move_to(cr, 8.5, -2.5); cairo_line_to(cr, 10.5, -2.5);
        cairo_stroke(cr);
    } else {
        const double r = (s == PetState::Wake) ? 1.4 : 1.0;
        cairo_arc(cr, 6,  -2.5, r, 0, 2 * kPi); cairo_fill(cr);
        cairo_arc(cr, 10, -2.5, r, 0, 2 * kPi); cairo_fill(cr);
    }

    // Nose
    cairo_set_source_rgba(cr, 0.95, 0.55, 0.60, 1.0);
    cairo_move_to(cr, 7.4,  0.2);
    cairo_line_to(cr, 8.6,  0.2);
    cairo_line_to(cr, 8.0,  1.2);
    cairo_close_path(cr);
    cairo_fill(cr);

    // Mouth — small "w" tucked under the nose.
    cairo_set_source_rgba(cr, 0.20, 0.20, 0.20, 0.9);
    cairo_set_line_width(cr, 0.5);
    cairo_move_to(cr, 8.0, 1.5);
    cairo_curve_to(cr, 7.5, 2.2, 7.0, 2.2, 6.5, 1.5);
    cairo_stroke(cr);
    cairo_move_to(cr, 8.0, 1.5);
    cairo_curve_to(cr, 8.5, 2.2, 9.0, 2.2, 9.5, 1.5);
    cairo_stroke(cr);

    // Whiskers — fan out to the right of the head.
    cairo_set_line_width(cr, 0.35);
    cairo_set_source_rgba(cr, 0.30, 0.30, 0.30, 0.75);
    for (int i = -1; i <= 1; ++i) {
        cairo_move_to(cr, 10.5, 0.5 + i * 0.6);
        cairo_line_to(cr, 13.5, 0.0 + i * 1.1);
    }
    cairo_stroke(cr);
}

void draw_tail(cairo_t* cr, int frame, PetState s) {
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    cairo_set_line_width(cr, 2.6);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    const double wag = (s == PetState::Sleep) ? 0.0
                     : std::sin(frame * 0.45) * 2.5;
    // Tail off the back (-x) of the body, curling up.
    cairo_move_to(cr, -9, 4);
    cairo_curve_to(cr, -13, 1, -14, -3 + wag, -11, -7 + wag);
    cairo_stroke(cr);
    cairo_set_source_rgba(cr, 0.30, 0.30, 0.30, 0.6);
    cairo_set_line_width(cr, 0.6);
    cairo_move_to(cr, -9, 4);
    cairo_curve_to(cr, -13, 1, -14, -3 + wag, -11, -7 + wag);
    cairo_stroke(cr);
}

void draw_legs(cairo_t* cr, PetState s, int frame) {
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    cairo_set_line_width(cr, 2.4);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    if (s == PetState::Run) {
        const int phase = frame % 2;
        const double a = phase ?  2.0 : -2.0;
        const double b = phase ? -2.0 :  2.0;
        // Front legs (+x side, leading)
        cairo_move_to(cr, 6, 9); cairo_line_to(cr, 6 + a, 13.5); cairo_stroke(cr);
        cairo_move_to(cr, 3, 9); cairo_line_to(cr, 3 + b, 13.5); cairo_stroke(cr);
        // Back legs (-x side, trailing)
        cairo_move_to(cr, -4, 9); cairo_line_to(cr, -4 + a, 13.5); cairo_stroke(cr);
        cairo_move_to(cr, -7, 9); cairo_line_to(cr, -7 + b, 13.5); cairo_stroke(cr);
    } else {
        cairo_move_to(cr, 6, 9);  cairo_line_to(cr, 6, 13.0);  cairo_stroke(cr);
        cairo_move_to(cr, 3, 9);  cairo_line_to(cr, 3, 13.0);  cairo_stroke(cr);
        cairo_move_to(cr, -4, 9); cairo_line_to(cr, -4, 13.0); cairo_stroke(cr);
        cairo_move_to(cr, -7, 9); cairo_line_to(cr, -7, 13.0); cairo_stroke(cr);
    }
}

void draw_zzz(cairo_t* cr, int frame) {
    cairo_set_source_rgba(cr, 0.30, 0.50, 0.95, 0.85);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 7);
    const double bob = std::sin(frame * 0.25) * 0.6;
    // Z's float up and to the right of the curled cat.
    cairo_move_to(cr, 6, -10 + bob);
    cairo_show_text(cr, "Z");
    cairo_set_font_size(cr, 5);
    cairo_move_to(cr, 11, -6 + bob);
    cairo_show_text(cr, "z");
    cairo_set_font_size(cr, 3.5);
    cairo_move_to(cr, 14, -3 + bob);
    cairo_show_text(cr, "z");
}

// Curled-up cat in a basket. The cat is drawn as a series of overlapping
// shapes: an oval body curled into a near-circle, a head tucked against the
// flank, and the tail wrapped around the front. The basket is a woven
// half-ellipse the cat sits in.
void draw_basket_and_curled_cat(cairo_t* cr, int frame) {
    // ---- Basket (back half drawn first so it sits behind the cat) -----------
    // Back rim
    cairo_set_source_rgba(cr, 0.55, 0.40, 0.25, 1.0);
    cairo_save(cr);
    cairo_translate(cr, 0, 8);
    cairo_scale(cr, 1.0, 0.45);
    cairo_arc(cr, 0, 0, 16, kPi, 2 * kPi);
    cairo_restore(cr);
    cairo_fill(cr);

    // ---- Cat curled in a near-circle ---------------------------------------
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    // Curled body — a slightly squashed circle.
    path_ellipse(cr, 0, 4, 11, 7);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.30, 0.30, 0.30, 0.7);
    cairo_set_line_width(cr, 0.6);
    cairo_stroke(cr);

    // Head tucked into the side of the body.
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    cairo_arc(cr, -7, 4, 4.5, 0, 2 * kPi);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.30, 0.30, 0.30, 0.7);
    cairo_stroke(cr);

    // One ear poking up, the other folded.
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    cairo_move_to(cr, -10, 1);
    cairo_line_to(cr, -9, -3);
    cairo_line_to(cr, -7, 1);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_set_source_rgba(cr, 0.95, 0.65, 0.70, 0.8);
    cairo_move_to(cr, -9, 0);
    cairo_line_to(cr, -8.5, -2);
    cairo_line_to(cr, -7.5, 0);
    cairo_close_path(cr);
    cairo_fill(cr);

    // Closed eye — single short curved line on the visible side of the head.
    cairo_set_source_rgba(cr, 0.10, 0.10, 0.10, 1.0);
    cairo_set_line_width(cr, 0.7);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, -7.5, 4.0);
    cairo_curve_to(cr, -7.0, 4.6, -5.5, 4.6, -5.0, 4.0);
    cairo_stroke(cr);

    // Tail wrapped around the front of the body — comes from the back and
    // curls up over the paws.
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    cairo_set_line_width(cr, 2.6);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, 8, 6);
    cairo_curve_to(cr, 11, 4, 11, 0, 7, -1);
    cairo_curve_to(cr, 4, -1, 2, 2, 4, 4);
    cairo_stroke(cr);

    // A paw peeking under the chin.
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    cairo_arc(cr, -3, 9, 1.6, 0, 2 * kPi); cairo_fill(cr);
    cairo_arc(cr, 0, 10, 1.6, 0, 2 * kPi); cairo_fill(cr);

    // ---- Basket (front rim, drawn last so it overlaps the cat's belly) -----
    cairo_set_source_rgba(cr, 0.65, 0.48, 0.30, 1.0);
    cairo_save(cr);
    cairo_translate(cr, 0, 11);
    cairo_scale(cr, 1.0, 0.55);
    cairo_arc(cr, 0, 0, 16, 0, kPi);
    cairo_restore(cr);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.35, 0.22, 0.10, 0.9);
    cairo_set_line_width(cr, 0.6);
    cairo_stroke(cr);

    // Weave lines on the front rim for the basket look.
    cairo_set_source_rgba(cr, 0.40, 0.27, 0.13, 0.7);
    cairo_set_line_width(cr, 0.5);
    for (int i = -3; i <= 3; ++i) {
        const double x = i * 4.0;
        cairo_move_to(cr, x, 11);
        cairo_line_to(cr, x, 16);
        cairo_stroke(cr);
    }

    draw_zzz(cr, frame);
}

} // namespace

void draw_procedural_cat(cairo_t* cr,
                         double cx, double cy,
                         double scale,
                         PetState state,
                         Direction direction,
                         int frame) {
    cairo_save(cr);
    cairo_translate(cr, cx, cy);
    if (faces_west(direction)) cairo_scale(cr, -scale, scale);
    else                       cairo_scale(cr,  scale, scale);

    if (state == PetState::Sleep) {
        draw_basket_and_curled_cat(cr, frame);
    } else {
        fill_body_and_head(cr);
        draw_ears(cr);
        draw_face(cr, state);
        draw_tail(cr, frame, state);
        draw_legs(cr, state, frame);

        if (state == PetState::Scratch) {
            cairo_set_source_rgba(cr, 0.85, 0.20, 0.20, 0.85);
            cairo_set_line_width(cr, 0.6);
            for (int i = 0; i < 3; ++i) {
                const double y = -1 + i * 1.6;
                // Scratch marks on the wall in front of the cat (+x side).
                cairo_move_to(cr, 13, y);
                cairo_line_to(cr, 16, y);
                cairo_stroke(cr);
            }
        }
    }
    cairo_restore(cr);
}

} // namespace hyprneko
