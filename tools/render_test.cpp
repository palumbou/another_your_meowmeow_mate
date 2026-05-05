// Off-screen smoke test of the procedural cat + speech bubble. Build:
//   nix-shell -p pkg-config cairo gcc --run \
//     "g++ -std=c++20 -I include -o build/render_test \
//        tools/render_test.cpp src/animation/ProceduralCat.cpp \
//        src/animation/SpeechBubble.cpp src/pet/PetStateMachine.cpp \
//        \$(pkg-config --cflags --libs cairo)"
//   ./build/render_test cat.png
#include "aymm/ProceduralCat.hpp"
#include "aymm/SpeechBubble.hpp"
#include <cairo/cairo.h>
#include <cstdio>

int main(int argc, char** argv) {
    using namespace aymm;
    const int W = 520, H = 150;
    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, W, H);
    cairo_t* cr = cairo_create(surf);

    cairo_set_source_rgba(cr, 0.10, 0.10, 0.15, 1.0);
    cairo_paint(cr);

    struct Pose { PetState s; Direction d; const char* label; const char* speech; };
    Pose poses[] = {
        { PetState::Idle,    Direction::E,    "idle",    "Hi!"           },
        { PetState::Run,     Direction::E,    "run-E",   nullptr         },
        { PetState::Run,     Direction::W,    "run-W",   nullptr         },
        { PetState::Sleep,   Direction::None, "sleep",   nullptr         },
        { PetState::Wake,    Direction::E,    "wake",    "Break time!"   },
    };

    SpeechBubble bubble;
    for (int i = 0; i < 5; ++i) {
        const double cx = 70 + i * 96.0;
        const double cy = 90.0;
        draw_procedural_cat(cr, cx, cy, 2.6, poses[i].s, poses[i].d, i);

        if (poses[i].speech) {
            bubble.say(poses[i].speech, std::chrono::seconds(60));
            bubble.draw(cr, cx, cy - 30);
        }

        cairo_set_source_rgba(cr, 0.85, 0.85, 0.85, 1.0);
        cairo_set_font_size(cr, 10);
        cairo_move_to(cr, cx - 20, cy + 60);
        cairo_show_text(cr, poses[i].label);
    }

    const char* path = (argc > 1) ? argv[1] : "cat.png";
    cairo_surface_write_to_png(surf, path);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    std::printf("wrote %s\n", path);
    return 0;
}
