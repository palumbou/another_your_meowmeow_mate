#pragma once

#include "hyprneko/Config.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

typedef struct _cairo cairo_t;

namespace hyprneko {

struct Rect { int x = 0, y = 0, w = 0, h = 0; };

// One transparent wlr-layer-shell overlay per wl_output. The caller supplies
// a draw callback that paints onto a Cairo context covering each output. The
// pet position is expressed in compositor-global coordinates; per-output
// surfaces apply the right translation before invoking the draw callback so
// the user code only ever talks in global coords.
//
// `monitor=auto` (Config) creates one surface per detected wl_output. Any
// other value filters by wl_output name (e.g. "DP-1").
class OverlaySurface {
public:
    // (cr, surface_w, surface_h, origin_x, origin_y) — origin is the
    // compositor-coord position of this output's top-left corner.
    using DrawFn = std::function<void(cairo_t*, int, int, int, int)>;

    OverlaySurface();
    ~OverlaySurface();

    OverlaySurface(const OverlaySurface&)            = delete;
    OverlaySurface& operator=(const OverlaySurface&) = delete;

    bool connect(const Config& cfg, std::string& err);

    void set_draw(DrawFn fn) { draw_ = std::move(fn); }
    const DrawFn& draw_callback() const { return draw_; }

    int run();
    void quit() { should_quit_ = true; }

    int  display_fd() const;
    bool dispatch_pending();
    void schedule_redraw();

    // Bounding box of all bound outputs in compositor coordinates. Useful for
    // picking an initial pet position. Returns {0,0,0,0} until at least one
    // output has been configured.
    Rect compositor_bounds() const;

    // Rectangle of the first configured output (a reasonable "primary"
    // hint for cold-start positioning).
    Rect primary_output_rect() const;

    // Internal — exposed so the .cpp's free Wayland callbacks can reach state.
    struct OutputSurface;
    struct Impl;
    Impl* __impl_for_callbacks();

private:
    std::unique_ptr<Impl> impl_;
    DrawFn draw_;
    bool should_quit_ = false;
};

} // namespace hyprneko
