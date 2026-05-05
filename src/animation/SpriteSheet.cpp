#include "aymm/SpriteSheet.hpp"

#include <cairo/cairo.h>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace aymm {

namespace {

std::string trim(std::string s) {
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))  s.pop_back();
    return s;
}

} // namespace

SpriteSheet::~SpriteSheet() {
    if (surface_) cairo_surface_destroy(surface_);
}

// Sheet config format (sheet.conf, key=value):
//   image=neko.png
//   frame_w=32
//   frame_h=32
//   anim.idle = 0,0
//   anim.run_e = 0,1 ; 1,1 ; 2,1
//   anim.run_w = 0,1 ; 1,1 ; 2,1 ; mirror
std::unique_ptr<SpriteSheet> SpriteSheet::load(const std::filesystem::path& dir,
                                               std::string& err) {
    const auto cfg = dir / "sheet.conf";
    std::ifstream in(cfg);
    if (!in) { err = "missing sheet.conf in " + dir.string(); return nullptr; }

    auto sheet = std::unique_ptr<SpriteSheet>(new SpriteSheet());
    std::string image;
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string k = trim(line.substr(0, eq));
        const std::string v = trim(line.substr(eq + 1));

        if      (k == "image")   image = v;
        else if (k == "frame_w") sheet->frame_w_ = std::atoi(v.c_str());
        else if (k == "frame_h") sheet->frame_h_ = std::atoi(v.c_str());
        else if (k.rfind("anim.", 0) == 0) {
            AnimationFrames af;
            af.name = k.substr(5);
            std::stringstream ss(v);
            std::string tok;
            while (std::getline(ss, tok, ';')) {
                tok = trim(tok);
                if (tok == "mirror") { af.mirror_x = true; continue; }
                int c = 0, r = 0;
                if (std::sscanf(tok.c_str(), "%d,%d", &c, &r) == 2)
                    af.frames.emplace_back(c, r);
            }
            sheet->animations_[af.name] = std::move(af);
        }
    }

    if (image.empty()) { err = "sheet.conf missing image="; return nullptr; }
    const auto image_path = (dir / image).string();
    sheet->surface_ = cairo_image_surface_create_from_png(image_path.c_str());
    if (cairo_surface_status(sheet->surface_) != CAIRO_STATUS_SUCCESS) {
        err = "failed to load PNG: " + image_path;
        return nullptr;
    }
    return sheet;
}

const AnimationFrames* SpriteSheet::animation(std::string_view name) const {
    const auto it = animations_.find(std::string(name));
    return it == animations_.end() ? nullptr : &it->second;
}

} // namespace aymm
