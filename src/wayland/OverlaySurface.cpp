#include "hyprneko/OverlaySurface.hpp"

#include <algorithm>
#include <cairo/cairo.h>
#include <cstring>
#include <fcntl.h>
#include <linux/memfd.h>
#include <memory>
#include <poll.h>
#include <string>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>
#include <wayland-client.h>

#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "xdg-shell-client-protocol.h"

namespace hyprneko {

namespace {

int memfd_create_compat(const char* name, unsigned flags) {
#ifdef __NR_memfd_create
    return static_cast<int>(::syscall(__NR_memfd_create, name, flags));
#else
    (void)flags;
    char tmpl[] = "/tmp/hyprneko-XXXXXX";
    int fd = ::mkstemp(tmpl);
    if (fd >= 0) ::unlink(tmpl);
    return fd;
#endif
}

uint32_t layer_to_proto(OverlayLayer l) {
    switch (l) {
        case OverlayLayer::Background: return ZWLR_LAYER_SHELL_V1_LAYER_BACKGROUND;
        case OverlayLayer::Bottom:     return ZWLR_LAYER_SHELL_V1_LAYER_BOTTOM;
        case OverlayLayer::Top:        return ZWLR_LAYER_SHELL_V1_LAYER_TOP;
        case OverlayLayer::Overlay:    return ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
    }
    return ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY;
}

} // namespace

struct OverlaySurface::OutputSurface {
    OverlaySurface* parent = nullptr;
    wl_output*      output = nullptr;
    uint32_t        global_name = 0;
    std::string     name;

    int             origin_x = 0;
    int             origin_y = 0;
    int             phys_w   = 0;
    int             phys_h   = 0;
    bool            geom_done = false;

    wl_surface*              surface       = nullptr;
    zwlr_layer_surface_v1*   layer_surface = nullptr;
    wl_callback*             frame_cb      = nullptr;

    int  width  = 0;
    int  height = 0;
    bool configured = false;
    bool surface_built = false;

    int        shm_fd      = -1;
    uint8_t*   mapped      = nullptr;
    size_t     mapped_size = 0;
    wl_buffer* buffer      = nullptr;
};

struct OverlaySurface::Impl {
    wl_display*              display     = nullptr;
    wl_compositor*           compositor  = nullptr;
    wl_shm*                  shm         = nullptr;
    zwlr_layer_shell_v1*     layer_shell = nullptr;

    std::vector<std::unique_ptr<OutputSurface>> outputs;
    OverlayLayer  layer = OverlayLayer::Overlay;
    std::string   monitor_request = "auto";

    OverlaySurface* parent = nullptr;
};

namespace {

// Forward declarations.
void render_output_with(OverlaySurface::OutputSurface* o, OverlaySurface::Impl* impl);
void build_surface_for(OverlaySurface::OutputSurface* o, OverlaySurface::Impl* impl);

// Trampoline so the frame/configure callbacks (which only have an
// OutputSurface*) can reach the parent's private Impl.
struct ImplAccess {
    static OverlaySurface::Impl* impl_of(OverlaySurface* p) {
        return p->__impl_for_callbacks();
    }
};

// ---- wl_output listener -----------------------------------------------------

void output_geometry(void* data, wl_output*, int32_t x, int32_t y,
                     int32_t /*pw*/, int32_t /*ph*/, int32_t /*subpixel*/,
                     const char*, const char*, int32_t /*transform*/) {
    auto* o = static_cast<OverlaySurface::OutputSurface*>(data);
    o->origin_x = x;
    o->origin_y = y;
}
void output_mode(void* data, wl_output*, uint32_t flags,
                 int32_t w, int32_t h, int32_t) {
    if (!(flags & WL_OUTPUT_MODE_CURRENT)) return;
    auto* o = static_cast<OverlaySurface::OutputSurface*>(data);
    o->phys_w = w;
    o->phys_h = h;
}
void output_done(void* data, wl_output*) {
    auto* o = static_cast<OverlaySurface::OutputSurface*>(data);
    o->geom_done = true;
}
void output_scale(void*, wl_output*, int32_t) {}
void output_name(void* data, wl_output*, const char* name) {
    auto* o = static_cast<OverlaySurface::OutputSurface*>(data);
    if (name) o->name = name;
}
void output_description(void*, wl_output*, const char*) {}

const wl_output_listener kOutputListener = {
    output_geometry, output_mode, output_done, output_scale,
    output_name, output_description,
};

// ---- buffer + render --------------------------------------------------------

void buffer_release(void*, wl_buffer*) {}
const wl_buffer_listener kBufferListener = { buffer_release };

bool ensure_buffer(OverlaySurface::OutputSurface* o, OverlaySurface::Impl* impl) {
    if (o->width <= 0 || o->height <= 0) return false;
    const size_t stride = static_cast<size_t>(o->width) * 4;
    const size_t size   = stride * static_cast<size_t>(o->height);
    if (o->mapped && o->mapped_size == size) return true;

    if (o->buffer) { wl_buffer_destroy(o->buffer); o->buffer = nullptr; }
    if (o->mapped) { ::munmap(o->mapped, o->mapped_size); o->mapped = nullptr; o->mapped_size = 0; }
    if (o->shm_fd >= 0) { ::close(o->shm_fd); o->shm_fd = -1; }

    o->shm_fd = memfd_create_compat("hyprneko-buf", MFD_CLOEXEC);
    if (o->shm_fd < 0) return false;
    if (::ftruncate(o->shm_fd, static_cast<off_t>(size)) < 0) return false;
    o->mapped = static_cast<uint8_t*>(
        ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, o->shm_fd, 0));
    if (o->mapped == MAP_FAILED) { o->mapped = nullptr; return false; }
    o->mapped_size = size;

    wl_shm_pool* pool = wl_shm_create_pool(impl->shm, o->shm_fd, static_cast<int32_t>(size));
    o->buffer = wl_shm_pool_create_buffer(pool, 0, o->width, o->height,
                                          static_cast<int32_t>(stride),
                                          WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    wl_buffer_add_listener(o->buffer, &kBufferListener, nullptr);
    return true;
}

void frame_done(void* data, wl_callback* cb, uint32_t /*time*/) {
    auto* o = static_cast<OverlaySurface::OutputSurface*>(data);
    wl_callback_destroy(cb);
    o->frame_cb = nullptr;
    if (o->parent) render_output_with(o, ImplAccess::impl_of(o->parent));
}
const wl_callback_listener kFrameListener = { frame_done };

void render_output_with(OverlaySurface::OutputSurface* o, OverlaySurface::Impl* impl) {
    if (!o->configured) return;
    if (!ensure_buffer(o, impl)) return;

    cairo_surface_t* surf = cairo_image_surface_create_for_data(
        o->mapped, CAIRO_FORMAT_ARGB32, o->width, o->height, o->width * 4);
    cairo_t* cr = cairo_create(surf);

    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    if (o->parent) {
        const auto& draw = o->parent->draw_callback();
        if (draw) draw(cr, o->width, o->height, o->origin_x, o->origin_y);
    }

    cairo_destroy(cr);
    cairo_surface_finish(surf);
    cairo_surface_destroy(surf);

    wl_surface_attach(o->surface, o->buffer, 0, 0);
    wl_surface_damage_buffer(o->surface, 0, 0, o->width, o->height);

    o->frame_cb = wl_surface_frame(o->surface);
    wl_callback_add_listener(o->frame_cb, &kFrameListener, o);

    wl_surface_commit(o->surface);

    // Crucial: push the commit (and its frame_cb listener registration) to
    // the compositor immediately. Without this the compositor never sees our
    // updates between poll() wake-ups, which means it never sends a frame
    // callback, which means we never re-render — total stall.
    wl_display_flush(impl->display);
}


void layer_configure(void* data, zwlr_layer_surface_v1* surface,
                     uint32_t serial, uint32_t width, uint32_t height) {
    auto* o = static_cast<OverlaySurface::OutputSurface*>(data);
    o->width      = static_cast<int>(width);
    o->height     = static_cast<int>(height);
    o->configured = true;
    zwlr_layer_surface_v1_ack_configure(surface, serial);
    if (o->parent) {
        auto* impl = ImplAccess::impl_of(o->parent);
        render_output_with(o, impl);
    }
}

void layer_closed(void* data, zwlr_layer_surface_v1*) {
    auto* o = static_cast<OverlaySurface::OutputSurface*>(data);
    if (o->parent) o->parent->quit();
}

const zwlr_layer_surface_v1_listener kLayerListener = { layer_configure, layer_closed };

void build_surface_for(OverlaySurface::OutputSurface* o, OverlaySurface::Impl* impl) {
    if (o->surface_built) return;
    if (!o->geom_done) return;
    o->surface_built = true;

    o->surface = wl_compositor_create_surface(impl->compositor);

    // Empty input region — overlay never steals input.
    wl_region* empty = wl_compositor_create_region(impl->compositor);
    wl_surface_set_input_region(o->surface, empty);
    wl_region_destroy(empty);

    o->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        impl->layer_shell, o->surface, o->output,
        layer_to_proto(impl->layer), "hyprneko");

    zwlr_layer_surface_v1_set_anchor(o->layer_surface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP    | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
      | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT   | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_size(o->layer_surface, 0, 0);
    zwlr_layer_surface_v1_set_exclusive_zone(o->layer_surface, -1);
    zwlr_layer_surface_v1_set_keyboard_interactivity(
        o->layer_surface, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
    zwlr_layer_surface_v1_add_listener(o->layer_surface, &kLayerListener, o);

    wl_surface_commit(o->surface);
}

// ---- registry ---------------------------------------------------------------

void registry_global(void* data, wl_registry* reg, uint32_t name,
                     const char* interface, uint32_t version) {
    auto* p = static_cast<OverlaySurface::Impl*>(data);
    if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
        p->compositor = static_cast<wl_compositor*>(
            wl_registry_bind(reg, name, &wl_compositor_interface, std::min<uint32_t>(version, 4)));
    } else if (std::strcmp(interface, wl_shm_interface.name) == 0) {
        p->shm = static_cast<wl_shm*>(wl_registry_bind(reg, name, &wl_shm_interface, 1));
    } else if (std::strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
        p->layer_shell = static_cast<zwlr_layer_shell_v1*>(
            wl_registry_bind(reg, name, &zwlr_layer_shell_v1_interface,
                             std::min<uint32_t>(version, 4)));
    } else if (std::strcmp(interface, wl_output_interface.name) == 0) {
        auto o = std::make_unique<OverlaySurface::OutputSurface>();
        o->parent      = p->parent;
        o->global_name = name;
        o->output      = static_cast<wl_output*>(
            wl_registry_bind(reg, name, &wl_output_interface, std::min<uint32_t>(version, 4)));
        wl_output_add_listener(o->output, &kOutputListener, o.get());
        p->outputs.push_back(std::move(o));
    }
}
void registry_global_remove(void*, wl_registry*, uint32_t) {}
const wl_registry_listener kRegistryListener = { registry_global, registry_global_remove };

} // namespace

// ---- OverlaySurface methods -------------------------------------------------

OverlaySurface::OverlaySurface() : impl_(std::make_unique<Impl>()) { impl_->parent = this; }

OverlaySurface::~OverlaySurface() {
    if (!impl_) return;
    for (auto& o : impl_->outputs) {
        if (o->frame_cb)      wl_callback_destroy(o->frame_cb);
        if (o->layer_surface) zwlr_layer_surface_v1_destroy(o->layer_surface);
        if (o->surface)       wl_surface_destroy(o->surface);
        if (o->buffer)        wl_buffer_destroy(o->buffer);
        if (o->mapped)        ::munmap(o->mapped, o->mapped_size);
        if (o->shm_fd >= 0)   ::close(o->shm_fd);
    }
    if (impl_->display) wl_display_disconnect(impl_->display);
}

bool OverlaySurface::connect(const Config& cfg, std::string& err) {
    impl_->display = wl_display_connect(nullptr);
    if (!impl_->display) { err = "wl_display_connect failed"; return false; }
    impl_->layer           = cfg.overlay_layer;
    impl_->monitor_request = cfg.monitor;

    wl_registry* reg = wl_display_get_registry(impl_->display);
    wl_registry_add_listener(reg, &kRegistryListener, impl_.get());
    wl_display_roundtrip(impl_->display);   // globals announced
    wl_display_roundtrip(impl_->display);   // wl_output events arrive

    if (!impl_->compositor || !impl_->shm || !impl_->layer_shell) {
        err = "compositor missing required globals (compositor/shm/wlr-layer-shell)";
        return false;
    }
    if (impl_->outputs.empty()) {
        err = "no wl_output bound from registry";
        return false;
    }

    // Filter outputs by name when a specific monitor was requested.
    if (cfg.monitor != "auto") {
        impl_->outputs.erase(
            std::remove_if(impl_->outputs.begin(), impl_->outputs.end(),
                [&](const std::unique_ptr<OutputSurface>& o){ return o->name != cfg.monitor; }),
            impl_->outputs.end());
        if (impl_->outputs.empty()) {
            err = "no wl_output matched monitor='" + cfg.monitor + "'";
            return false;
        }
    }

    for (auto& o : impl_->outputs) build_surface_for(o.get(), impl_.get());
    wl_display_roundtrip(impl_->display); // first configure events
    return true;
}

int OverlaySurface::display_fd() const {
    return impl_->display ? wl_display_get_fd(impl_->display) : -1;
}

bool OverlaySurface::dispatch_pending() {
    if (!impl_->display) return false;
    while (wl_display_prepare_read(impl_->display) != 0) {
        if (wl_display_dispatch_pending(impl_->display) < 0) return false;
    }
    if (wl_display_flush(impl_->display) < 0 && errno != EAGAIN) {
        wl_display_cancel_read(impl_->display);
        return false;
    }
    pollfd pfd{ wl_display_get_fd(impl_->display), POLLIN, 0 };
    if (::poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN)) {
        if (wl_display_read_events(impl_->display) < 0) return false;
    } else {
        wl_display_cancel_read(impl_->display);
    }
    return wl_display_dispatch_pending(impl_->display) >= 0;
}

void OverlaySurface::schedule_redraw() {
    for (auto& o : impl_->outputs) {
        if (!o->frame_cb) render_output_with(o.get(), impl_.get());
    }
}

int OverlaySurface::run() {
    while (!should_quit_) {
        if (wl_display_dispatch(impl_->display) < 0) return 1;
    }
    return 0;
}

Rect OverlaySurface::compositor_bounds() const {
    Rect r{};
    bool seen = false;
    for (const auto& o : impl_->outputs) {
        if (!o->configured) continue;
        const int ow = o->width  > 0 ? o->width  : o->phys_w;
        const int oh = o->height > 0 ? o->height : o->phys_h;
        if (!seen) {
            r = Rect{ o->origin_x, o->origin_y, ow, oh };
            seen = true;
        } else {
            const int x1 = std::min(r.x, o->origin_x);
            const int y1 = std::min(r.y, o->origin_y);
            const int x2 = std::max(r.x + r.w, o->origin_x + ow);
            const int y2 = std::max(r.y + r.h, o->origin_y + oh);
            r = Rect{ x1, y1, x2 - x1, y2 - y1 };
        }
    }
    return r;
}

Rect OverlaySurface::primary_output_rect() const {
    for (const auto& o : impl_->outputs) {
        if (!o->configured) continue;
        const int ow = o->width  > 0 ? o->width  : o->phys_w;
        const int oh = o->height > 0 ? o->height : o->phys_h;
        return Rect{ o->origin_x, o->origin_y, ow, oh };
    }
    return Rect{};
}

OverlaySurface::Impl* OverlaySurface::__impl_for_callbacks() {
    return impl_.get();
}

} // namespace hyprneko
