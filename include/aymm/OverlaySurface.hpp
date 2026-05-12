#pragma once

#include "aymm/Config.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

typedef struct _cairo cairo_t;

namespace aymm {

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

    // (button_code, gx, gy) — button_code uses Linux input-event-codes
    // (BTN_LEFT=0x110, BTN_RIGHT=0x111, BTN_MIDDLE=0x112). (gx, gy) is the
    // compositor-global pointer position when the button was pressed.
    using ButtonFn = std::function<void(uint32_t, double, double)>;
    void set_button_callback(ButtonFn fn) { button_cb_ = std::move(fn); }
    const ButtonFn& button_callback() const { return button_cb_; }

    int run();
    void quit() { should_quit_ = true; }

    int  display_fd() const;
    bool dispatch_pending();
    void schedule_redraw();

    // Define a small input-capture rectangle (in compositor-global coords)
    // that aymm wants to receive pointer events on. The actual region is
    // applied per-output: outputs whose rectangle doesn't intersect the cat
    // get an empty input region (full click-through). Call this each tick
    // with the current cat center; OverlaySurface throttles internally so
    // we don't commit the surface unnecessarily.
    void set_cat_region(int gx, int gy, int size_px);

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
    DrawFn   draw_;
    ButtonFn button_cb_;
    bool should_quit_ = false;
};

} // namespace aymm
