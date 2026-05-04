#include "hyprneko/OverlaySurface.hpp"

#include <algorithm>
#include <cairo/cairo.h>
#include <cstring>
#include <fcntl.h>
#include <linux/memfd.h>
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

struct OutputBinding {
    wl_output*  output = nullptr;
    uint32_t    name   = 0;
    std::string description;
};

struct OverlaySurface::Impl {
    wl_display*              display       = nullptr;
    wl_compositor*           compositor    = nullptr;
    wl_shm*                  shm           = nullptr;
    zwlr_layer_shell_v1*     layer_shell   = nullptr;
    std::vector<OutputBinding> outputs;

    wl_output*               selected_output = nullptr;
    std::string              monitor_request = "auto";

    wl_surface*              surface       = nullptr;
    zwlr_layer_surface_v1*   layer_surface = nullptr;
    wl_callback*             frame_cb      = nullptr;

    int  width  = 0;
    int  height = 0;
    bool configured = false;

    // Buffer pool — re-allocated on resize.
    int      shm_fd       = -1;
    uint8_t* mapped       = nullptr;
    size_t   mapped_size  = 0;
    wl_buffer* buffer     = nullptr;

    OverlaySurface* parent = nullptr;
};

namespace {

// ---- listeners --------------------------------------------------------------

void output_geometry(void*, wl_output*, int32_t, int32_t, int32_t, int32_t,
                     int32_t, const char*, const char*, int32_t) {}
void output_mode(void*, wl_output*, uint32_t, int32_t, int32_t, int32_t) {}
void output_done(void*, wl_output*) {}
void output_scale(void*, wl_output*, int32_t) {}
void output_name(void* data, wl_output* out, const char* name) {
    auto* ob = static_cast<OutputBinding*>(data);
    if (ob && name) ob->description = name;
    (void)out;
}
void output_description(void*, wl_output*, const char*) {}

const wl_output_listener kOutputListener = {
    output_geometry, output_mode, output_done, output_scale,
    output_name, output_description,
};

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
        OutputBinding ob;
        ob.name   = name;
        ob.output = static_cast<wl_output*>(
            wl_registry_bind(reg, name, &wl_output_interface, std::min<uint32_t>(version, 4)));
        p->outputs.push_back(ob);
        wl_output_add_listener(p->outputs.back().output, &kOutputListener, &p->outputs.back());
    }
}

void registry_global_remove(void*, wl_registry*, uint32_t) {}

const wl_registry_listener kRegistryListener = {
    registry_global, registry_global_remove,
};

// Forward-declared so configure_handler can reference render.
void render(OverlaySurface::Impl* p, OverlaySurface* parent);

void layer_configure(void* data, zwlr_layer_surface_v1* surface,
                     uint32_t serial, uint32_t width, uint32_t height) {
    auto* impl = static_cast<OverlaySurface::Impl*>(data);
    impl->width      = static_cast<int>(width);
    impl->height     = static_cast<int>(height);
    impl->configured = true;
    zwlr_layer_surface_v1_ack_configure(surface, serial);
    render(impl, impl->parent);
}

void layer_closed(void* data, zwlr_layer_surface_v1*) {
    auto* impl = static_cast<OverlaySurface::Impl*>(data);
    if (impl->parent) impl->parent->quit();
}

const zwlr_layer_surface_v1_listener kLayerListener = {
    layer_configure, layer_closed,
};

void frame_done(void* data, wl_callback* cb, uint32_t /*time*/) {
    auto* impl = static_cast<OverlaySurface::Impl*>(data);
    wl_callback_destroy(cb);
    impl->frame_cb = nullptr;
    render(impl, impl->parent);
}

const wl_callback_listener kFrameListener = { frame_done };

void buffer_release(void*, wl_buffer*) {
    // Single-buffer redraw model — we rebuild on each frame so this is a no-op.
}

const wl_buffer_listener kBufferListener = { buffer_release };

// ---- buffer + render --------------------------------------------------------

bool ensure_buffer(OverlaySurface::Impl* p) {
    if (p->width <= 0 || p->height <= 0) return false;
    const size_t stride = static_cast<size_t>(p->width) * 4;
    const size_t size   = stride * static_cast<size_t>(p->height);
    if (p->mapped && p->mapped_size == size) return true;

    if (p->buffer) { wl_buffer_destroy(p->buffer); p->buffer = nullptr; }
    if (p->mapped) { ::munmap(p->mapped, p->mapped_size); p->mapped = nullptr; p->mapped_size = 0; }
    if (p->shm_fd >= 0) { ::close(p->shm_fd); p->shm_fd = -1; }

    p->shm_fd = memfd_create_compat("hyprneko-buf", MFD_CLOEXEC);
    if (p->shm_fd < 0) return false;
    if (::ftruncate(p->shm_fd, static_cast<off_t>(size)) < 0) return false;
    p->mapped = static_cast<uint8_t*>(
        ::mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, p->shm_fd, 0));
    if (p->mapped == MAP_FAILED) { p->mapped = nullptr; return false; }
    p->mapped_size = size;

    wl_shm_pool* pool = wl_shm_create_pool(p->shm, p->shm_fd, static_cast<int32_t>(size));
    p->buffer = wl_shm_pool_create_buffer(pool, 0, p->width, p->height,
                                          static_cast<int32_t>(stride),
                                          WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    wl_buffer_add_listener(p->buffer, &kBufferListener, nullptr);
    return true;
}

void render(OverlaySurface::Impl* p, OverlaySurface* parent) {
    if (!p->configured) return;
    if (!ensure_buffer(p)) return;

    cairo_surface_t* surf = cairo_image_surface_create_for_data(
        p->mapped, CAIRO_FORMAT_ARGB32, p->width, p->height,
        p->width * 4);
    cairo_t* cr = cairo_create(surf);

    // Clear to fully transparent.
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    if (parent) {
        const auto& draw = parent->draw_callback();
        if (draw) draw(cr, p->width, p->height);
    }

    cairo_destroy(cr);
    cairo_surface_finish(surf);
    cairo_surface_destroy(surf);

    wl_surface_attach(p->surface, p->buffer, 0, 0);
    wl_surface_damage_buffer(p->surface, 0, 0, p->width, p->height);

    p->frame_cb = wl_surface_frame(p->surface);
    wl_callback_add_listener(p->frame_cb, &kFrameListener, p);

    wl_surface_commit(p->surface);
}

} // namespace

// ---- OverlaySurface methods -------------------------------------------------

OverlaySurface::OverlaySurface() : impl_(std::make_unique<Impl>()) { impl_->parent = this; }

OverlaySurface::~OverlaySurface() {
    if (!impl_) return;
    // The objects bound from the registry (compositor/shm/layer_shell/outputs)
    // are reaped by wl_display_disconnect, so we only release the resources
    // that are not owned by the Wayland display itself.
    if (impl_->frame_cb)      wl_callback_destroy(impl_->frame_cb);
    if (impl_->layer_surface) zwlr_layer_surface_v1_destroy(impl_->layer_surface);
    if (impl_->surface)       wl_surface_destroy(impl_->surface);
    if (impl_->buffer)        wl_buffer_destroy(impl_->buffer);
    if (impl_->mapped)        ::munmap(impl_->mapped, impl_->mapped_size);
    if (impl_->shm_fd >= 0)   ::close(impl_->shm_fd);
    if (impl_->display)       wl_display_disconnect(impl_->display);
}

bool OverlaySurface::connect(const Config& cfg, std::string& err) {
    impl_->display = wl_display_connect(nullptr);
    if (!impl_->display) { err = "wl_display_connect failed"; return false; }

    wl_registry* reg = wl_display_get_registry(impl_->display);
    wl_registry_add_listener(reg, &kRegistryListener, impl_.get());
    wl_display_roundtrip(impl_->display); // first round: globals announced
    wl_display_roundtrip(impl_->display); // second: output names arrive

    if (!impl_->compositor || !impl_->shm || !impl_->layer_shell) {
        err = "compositor missing required globals (compositor/shm/wlr-layer-shell)";
        return false;
    }

    impl_->monitor_request = cfg.monitor;
    if (cfg.monitor != "auto") {
        for (auto& ob : impl_->outputs) {
            if (ob.description == cfg.monitor) { impl_->selected_output = ob.output; break; }
        }
    }

    impl_->surface = wl_compositor_create_surface(impl_->compositor);

    // Pass-through input: empty input region.
    wl_region* empty = wl_compositor_create_region(impl_->compositor);
    wl_surface_set_input_region(impl_->surface, empty);
    wl_region_destroy(empty);

    impl_->layer_surface = zwlr_layer_shell_v1_get_layer_surface(
        impl_->layer_shell, impl_->surface, impl_->selected_output,
        layer_to_proto(cfg.overlay_layer), "hyprneko");

    zwlr_layer_surface_v1_set_anchor(impl_->layer_surface,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP    | ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM
      | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT   | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_size(impl_->layer_surface, 0, 0);
    zwlr_layer_surface_v1_set_exclusive_zone(impl_->layer_surface, -1);
    zwlr_layer_surface_v1_set_keyboard_interactivity(
        impl_->layer_surface, ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);

    zwlr_layer_surface_v1_add_listener(impl_->layer_surface, &kLayerListener, impl_.get());

    wl_surface_commit(impl_->surface);
    wl_display_roundtrip(impl_->display);

    width_  = impl_->width;
    height_ = impl_->height;
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
    if (!impl_->frame_cb) render(impl_.get(), this);
}

int OverlaySurface::run() {
    while (!should_quit_) {
        if (wl_display_dispatch(impl_->display) < 0) return 1;
    }
    return 0;
}

} // namespace hyprneko
