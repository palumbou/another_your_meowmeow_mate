#pragma once

#include "hyprneko/Config.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

typedef struct _cairo cairo_t;
struct wl_display;
struct wl_compositor;
struct wl_shm;
struct wl_output;
struct wl_surface;
struct wl_buffer;
struct wl_callback;
struct zwlr_layer_shell_v1;
struct zwlr_layer_surface_v1;

namespace hyprneko {

// One full-monitor transparent layer-shell overlay. The caller supplies a
// draw callback that paints onto a Cairo context covering the surface.
//
// v1 limitation: single output (config.monitor selects by name; "auto"
// lets the compositor pick). Multi-output is a planned follow-up.
class OverlaySurface {
public:
    using DrawFn = std::function<void(cairo_t*, int width, int height)>;

    OverlaySurface();
    ~OverlaySurface();

    OverlaySurface(const OverlaySurface&)            = delete;
    OverlaySurface& operator=(const OverlaySurface&) = delete;

    // Connect to compositor and create the layer surface. Returns false on
    // failure; the error string is filled on the way out.
    bool connect(const Config& cfg, std::string& err);

    void set_draw(DrawFn fn) { draw_ = std::move(fn); }
    const DrawFn& draw_callback() const { return draw_; }

    // Drive Wayland: read pending events, dispatch, paint frames as the
    // compositor pacing requests them. Returns when wl_display_dispatch
    // fails or `should_quit()` becomes true.
    int run();

    void quit() { should_quit_ = true; }

    // Query monitor logical size (compositor coordinates); valid after the
    // first configure event.
    int width()  const { return width_;  }
    int height() const { return height_; }

    // Native fd of the wl_display, for poll() integration.
    int display_fd() const;

    // Pump events without blocking. Returns false if the connection died.
    bool dispatch_pending();

    // Request the compositor to send a new frame callback (forces a redraw
    // soon). Idempotent if a callback is already armed.
    void schedule_redraw();

    // Exposed so the .cpp's Wayland callback functions (which must be free
    // C-linkage helpers) can reach internal state. Treat as private API.
    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
    DrawFn draw_;
    int  width_  = 0;
    int  height_ = 0;
    bool should_quit_ = false;
};

} // namespace hyprneko
