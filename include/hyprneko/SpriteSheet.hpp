#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declare Cairo to avoid leaking the header through public includes.
typedef struct _cairo_surface cairo_surface_t;

namespace hyprneko {

// One animation = ordered list of (col, row) frames pulled from the sheet.
struct AnimationFrames {
    std::string         name;
    std::vector<std::pair<int,int>> frames; // (col, row) per frame
    bool                mirror_x = false;
};

// Sprite sheet loader (PNG via Cairo). Maps logical animation names like
// "run_n", "scratch", "sleep" to a frame list. The mapping comes from a
// sidecar file `sheet.conf` next to the PNG.
class SpriteSheet {
public:
    static std::unique_ptr<SpriteSheet> load(const std::filesystem::path& dir,
                                             std::string& err);
    ~SpriteSheet();

    cairo_surface_t* surface() const { return surface_; }
    int frame_w() const { return frame_w_; }
    int frame_h() const { return frame_h_; }

    const AnimationFrames* animation(std::string_view name) const;

private:
    SpriteSheet() = default;

    cairo_surface_t* surface_ = nullptr;
    int frame_w_ = 32;
    int frame_h_ = 32;
    std::unordered_map<std::string, AnimationFrames> animations_;
};

} // namespace hyprneko
