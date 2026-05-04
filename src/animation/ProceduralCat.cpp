#include "hyprneko/ProceduralCat.hpp"

#include <cairo/cairo.h>
#include <cmath>

namespace hyprneko {

namespace {

constexpr double kPi = 3.14159265358979323846;

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

void fill_body_and_head(cairo_t* cr) {
    // Pure white fur, soft gray outline.
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);

    // Body
    path_ellipse(cr, 0, 4, 9, 6);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.30, 0.30, 0.30, 0.85);
    cairo_set_line_width(cr, 0.6);
    cairo_stroke(cr);

    // Head
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    cairo_arc(cr, -8, -2, 5.5, 0, 2 * kPi);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.30, 0.30, 0.30, 0.85);
    cairo_stroke(cr);
}

void draw_ears(cairo_t* cr) {
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);

    // Left ear
    cairo_move_to(cr, -12, -5);
    cairo_line_to(cr, -11, -10);
    cairo_line_to(cr, -8,  -4);
    cairo_close_path(cr);
    cairo_fill(cr);

    // Right ear
    cairo_move_to(cr, -8,  -7);
    cairo_line_to(cr, -5, -10);
    cairo_line_to(cr, -4,  -4);
    cairo_close_path(cr);
    cairo_fill(cr);

    // Pink inner ears
    cairo_set_source_rgba(cr, 0.95, 0.65, 0.70, 0.9);
    cairo_move_to(cr, -11, -6);
    cairo_line_to(cr, -10, -8);
    cairo_line_to(cr,  -9, -5);
    cairo_close_path(cr);
    cairo_fill(cr);
    cairo_move_to(cr, -7, -6);
    cairo_line_to(cr, -6, -8);
    cairo_line_to(cr, -5, -5);
    cairo_close_path(cr);
    cairo_fill(cr);
}

void draw_face(cairo_t* cr, PetState s) {
    // Eyes
    cairo_set_source_rgba(cr, 0.10, 0.10, 0.10, 1.0);
    if (s == PetState::Sleep) {
        cairo_set_line_width(cr, 0.8);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_move_to(cr, -10.5, -2.5); cairo_line_to(cr,  -8.5, -2.5);
        cairo_stroke(cr);
        cairo_move_to(cr,  -7.5, -2.5); cairo_line_to(cr,  -5.5, -2.5);
        cairo_stroke(cr);
    } else {
        const double r = (s == PetState::Wake) ? 1.3 : 1.0;
        cairo_arc(cr, -10, -2.5, r, 0, 2 * kPi); cairo_fill(cr);
        cairo_arc(cr,  -6, -2.5, r, 0, 2 * kPi); cairo_fill(cr);
    }

    // Nose
    cairo_set_source_rgba(cr, 0.95, 0.55, 0.60, 1.0);
    cairo_move_to(cr, -8.6,  0.2);
    cairo_line_to(cr, -7.4,  0.2);
    cairo_line_to(cr, -8.0,  1.2);
    cairo_close_path(cr);
    cairo_fill(cr);

    // Mouth
    cairo_set_source_rgba(cr, 0.20, 0.20, 0.20, 0.9);
    cairo_set_line_width(cr, 0.5);
    cairo_move_to(cr, -8.0, 1.5);
    cairo_curve_to(cr, -8.5, 2.2, -9.0, 2.2, -9.5, 1.5);
    cairo_stroke(cr);
    cairo_move_to(cr, -8.0, 1.5);
    cairo_curve_to(cr, -7.5, 2.2, -7.0, 2.2, -6.5, 1.5);
    cairo_stroke(cr);

    // Whiskers
    cairo_set_line_width(cr, 0.35);
    cairo_set_source_rgba(cr, 0.30, 0.30, 0.30, 0.7);
    for (int i = -1; i <= 1; ++i) {
        cairo_move_to(cr, -10.5, 0.5 + i * 0.6); cairo_line_to(cr, -13.5, 0.0 + i * 1.0);
        cairo_move_to(cr,  -5.5, 0.5 + i * 0.6); cairo_line_to(cr,  -2.5, 0.0 + i * 1.0);
    }
    cairo_stroke(cr);
}

void draw_tail(cairo_t* cr, int frame, PetState s) {
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    cairo_set_line_width(cr, 2.6);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    const double wag = (s == PetState::Sleep) ? 0.0
                     : std::sin(frame * 0.45) * 2.5;
    cairo_move_to(cr, 9, 4);
    cairo_curve_to(cr, 13, 1, 14, -3 + wag, 11, -7 + wag);
    cairo_stroke(cr);
    cairo_set_source_rgba(cr, 0.30, 0.30, 0.30, 0.6);
    cairo_set_line_width(cr, 0.6);
    cairo_move_to(cr, 9, 4);
    cairo_curve_to(cr, 13, 1, 14, -3 + wag, 11, -7 + wag);
    cairo_stroke(cr);
}

void draw_legs(cairo_t* cr, PetState s, int frame) {
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    cairo_set_line_width(cr, 2.4);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

    if (s == PetState::Run) {
        const int phase = (frame / 1) % 2;       // alternate every tick
        const double a = phase ?  2.0 : -2.0;
        const double b = phase ? -2.0 :  2.0;
        // front legs
        cairo_move_to(cr, -6, 9); cairo_line_to(cr, -6 + a, 13.5); cairo_stroke(cr);
        cairo_move_to(cr, -3, 9); cairo_line_to(cr, -3 + b, 13.5); cairo_stroke(cr);
        // back legs
        cairo_move_to(cr,  4, 9); cairo_line_to(cr,  4 + a, 13.5); cairo_stroke(cr);
        cairo_move_to(cr,  7, 9); cairo_line_to(cr,  7 + b, 13.5); cairo_stroke(cr);
    } else {
        // Standing — four short straight legs
        cairo_move_to(cr, -6, 9); cairo_line_to(cr, -6, 13.0); cairo_stroke(cr);
        cairo_move_to(cr, -3, 9); cairo_line_to(cr, -3, 13.0); cairo_stroke(cr);
        cairo_move_to(cr,  4, 9); cairo_line_to(cr,  4, 13.0); cairo_stroke(cr);
        cairo_move_to(cr,  7, 9); cairo_line_to(cr,  7, 13.0); cairo_stroke(cr);
    }
}

void draw_zzz(cairo_t* cr, int frame) {
    cairo_set_source_rgba(cr, 0.30, 0.50, 0.95, 0.85);
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 6);
    const double bob = std::sin(frame * 0.25) * 0.6;
    cairo_move_to(cr, 6, -7 + bob);
    cairo_show_text(cr, "Z");
    cairo_set_font_size(cr, 4);
    cairo_move_to(cr, 10, -3 + bob);
    cairo_show_text(cr, "z");
}

void draw_lying_cat(cairo_t* cr, int frame) {
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    path_ellipse(cr, 0, 6, 12, 4);
    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.30, 0.30, 0.30, 0.7);
    cairo_set_line_width(cr, 0.6);
    cairo_stroke(cr);

    // Curled head and one paw
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    cairo_arc(cr, -9, 4, 4, 0, 2 * kPi);
    cairo_fill(cr);

    // Closed eyes
    cairo_set_source_rgba(cr, 0.10, 0.10, 0.10, 1.0);
    cairo_set_line_width(cr, 0.6);
    cairo_move_to(cr, -10.5, 4.0); cairo_line_to(cr, -8.5, 4.0); cairo_stroke(cr);

    // Tail curled around the body
    cairo_set_source_rgba(cr, 0.96, 0.96, 0.96, 1.0);
    cairo_set_line_width(cr, 2.4);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, 11, 5);
    cairo_curve_to(cr, 13, 8, 8, 11, 4, 9);
    cairo_stroke(cr);

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
    const double s = scale;
    if (faces_west(direction)) cairo_scale(cr, -s, s);
    else                       cairo_scale(cr,  s, s);

    if (state == PetState::Sleep) {
        draw_lying_cat(cr, frame);
    } else {
        fill_body_and_head(cr);
        draw_ears(cr);
        draw_face(cr, state);
        draw_tail(cr, frame, state);
        draw_legs(cr, state, frame);

        // Tiny "scratch" lines when in scratch state
        if (state == PetState::Scratch) {
            cairo_set_source_rgba(cr, 0.85, 0.20, 0.20, 0.85);
            cairo_set_line_width(cr, 0.6);
            for (int i = 0; i < 3; ++i) {
                const double y = -1 + i * 1.6;
                cairo_move_to(cr, -16, y);
                cairo_line_to(cr, -13, y);
                cairo_stroke(cr);
            }
        }
    }
    cairo_restore(cr);
}

} // namespace hyprneko
