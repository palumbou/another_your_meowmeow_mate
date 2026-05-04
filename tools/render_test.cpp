// Standalone smoke test: paints the procedural cat into a PNG so we can
// visually verify it renders without needing a Wayland session. Build:
//   nix-shell -p cmake pkg-config cairo gcc --run \
//     "g++ -std=c++20 -I include -o build/render_test \
//        tools/render_test.cpp src/animation/ProceduralCat.cpp \
//        src/pet/PetStateMachine.cpp \
//        \$(pkg-config --cflags --libs cairo)"
//   ./build/render_test cat.png
#include "hyprneko/ProceduralCat.hpp"
#include <cairo/cairo.h>
#include <cstdio>

int main(int argc, char** argv) {
    using namespace hyprneko;
    const int W = 320, H = 80;
    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, W, H);
    cairo_t* cr = cairo_create(surf);

    cairo_set_source_rgba(cr, 0.10, 0.10, 0.15, 1.0);
    cairo_paint(cr);

    struct Pose { PetState s; Direction d; const char* label; };
    Pose poses[] = {
        { PetState::Idle,    Direction::E, "idle"   },
        { PetState::Run,     Direction::E, "run-E"  },
        { PetState::Run,     Direction::W, "run-W"  },
        { PetState::Sleep,   Direction::None, "sleep" },
        { PetState::Scratch, Direction::E, "scratch" },
    };
    for (int i = 0; i < 5; ++i) {
        const double cx = 32 + i * 64.0;
        const double cy = 40.0;
        draw_procedural_cat(cr, cx, cy, 1.6, poses[i].s, poses[i].d, i);

        cairo_set_source_rgba(cr, 0.85, 0.85, 0.85, 1.0);
        cairo_set_font_size(cr, 8);
        cairo_move_to(cr, cx - 14, cy + 28);
        cairo_show_text(cr, poses[i].label);
    }
    const char* path = (argc > 1) ? argv[1] : "cat.png";
    cairo_surface_write_to_png(surf, path);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    std::printf("wrote %s\n", path);
    return 0;
}
