#include "hyprneko/App.hpp"
#include "hyprneko/UserPersona.hpp"
#include "hyprneko/WaybarStatus.hpp"

#include <cairo/cairo.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <poll.h>
#include <sys/timerfd.h>
#include <unistd.h>

namespace hyprneko {

namespace {

std::unique_ptr<CursorProvider> make_cursor_provider(CursorSource s) {
    switch (s) {
        case CursorSource::Hyprland: {
            auto p = std::make_unique<HyprlandCursorProvider>();
            if (p->ready()) return p;
            std::cerr << "hyprneko: Hyprland cursor socket not detected; "
                         "falling back to null provider.\n";
            return std::make_unique<NullCursorProvider>();
        }
        case CursorSource::Wlroots:
        case CursorSource::Evdev:
        case CursorSource::Null:
        default:
            return std::make_unique<NullCursorProvider>();
    }
}

} // namespace

App::App(Config cfg)
  : cfg_(std::move(cfg)),
    pomodoro_(PomodoroConfig{
        cfg_.pomodoro_study_minutes,
        cfg_.pomodoro_break_minutes,
        cfg_.pomodoro_long_break_minutes,
        cfg_.pomodoro_sessions_before_long_break,
        cfg_.pomodoro_auto_start,
        cfg_.pomodoro_show_notifications}),
    pomodoro_behavior_(pet_, pomodoro_, cfg_.movement_speed, cfg_.idle_distance),
    cursor_(make_cursor_provider(cfg_.cursor_source)),
    chase_(std::make_unique<CursorChaseBehavior>(pet_, *cursor_))
{
    pet_.speed         = cfg_.movement_speed;
    pet_.idle_distance = cfg_.idle_distance;

    if (!cfg_.sprite_dir.empty()) {
        std::string err;
        sheet_ = SpriteSheet::load(cfg_.sprite_dir, err);
        if (sheet_) {
            animations_ = std::make_unique<AnimationResolver>(*sheet_);
        } else {
            std::cerr << "hyprneko: sprite sheet load failed: " << err
                      << "  (drawing placeholder).\n";
        }
    }

    if (cfg_.enable_pomodoro && cfg_.pomodoro_auto_start) pomodoro_.start();
}

int App::run() {
    std::string err;
    if (!overlay_.connect(cfg_, err)) {
        std::cerr << "hyprneko: " << err << "\n";
        return 1;
    }

    // Center pet on first frame.
    pet_.position.x = overlay_.width()  / 2.0;
    pet_.position.y = overlay_.height() / 2.0;

    overlay_.set_draw([this](cairo_t* cr, int w, int h){ draw(cr, w, h); });

    if (control_.bind_listen() < 0) {
        std::cerr << "hyprneko: control socket bind failed (errno="
                  << errno << "); CLI commands will not reach this daemon.\n";
    }

    const int tick_hz = std::max(1, cfg_.cursor_poll_hz);
    const long ns_per_tick = 1'000'000'000L / tick_hz;
    const int tfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    itimerspec spec{};
    spec.it_value.tv_nsec    = ns_per_tick;
    spec.it_interval.tv_nsec = ns_per_tick;
    ::timerfd_settime(tfd, 0, &spec, nullptr);

    const int wl_fd  = overlay_.display_fd();
    const int ctl_fd = control_.listen_fd();

    while (true) {
        pollfd pfds[3];
        int n = 0;
        pfds[n++] = pollfd{ wl_fd,  POLLIN, 0 };
        pfds[n++] = pollfd{ tfd,    POLLIN, 0 };
        if (ctl_fd >= 0) pfds[n++] = pollfd{ ctl_fd, POLLIN, 0 };

        if (::poll(pfds, n, 1000) < 0) {
            if (errno == EINTR) continue;
            break;
        }

        if (pfds[0].revents & POLLIN) {
            if (!overlay_.dispatch_pending()) break;
        }
        if (pfds[1].revents & POLLIN) {
            uint64_t exp;
            (void)!::read(tfd, &exp, sizeof(exp));
            on_tick();
            overlay_.schedule_redraw();
        }
        if (n >= 3 && (pfds[2].revents & POLLIN)) {
            control_.accept_one([this](const std::string& r){ return handle_request(r); });
        }
    }

    ::close(tfd);
    return 0;
}

void App::on_tick() {
    pomodoro_.tick(PomodoroTimer::Clock::now());
    if (cfg_.enable_pomodoro) pomodoro_behavior_.apply();
    if (cfg_.behavior == Behavior::CursorChase) {
        chase_->update(Pet::Clock::now());
    } else {
        pet_.has_target = false;
        pet_.step(Pet::Clock::now());
    }
}

void App::draw(cairo_t* cr, int w, int h) {
    if (!pet_visible_) return;

    if (sheet_ && animations_) {
        const auto* af = animations_->resolve(pet_.fsm.state(), pet_.fsm.direction());
        if (af && !af->frames.empty()) {
            const auto [col, row] = af->frames[pet_.fsm.frame() % af->frames.size()];
            cairo_save(cr);
            const double dx = pet_.position.x - sheet_->frame_w() / 2.0;
            const double dy = pet_.position.y - sheet_->frame_h() / 2.0;
            cairo_translate(cr, dx, dy);
            if (af->mirror_x) {
                cairo_scale(cr, -1, 1);
                cairo_translate(cr, -static_cast<double>(sheet_->frame_w()), 0);
            }
            cairo_set_source_surface(cr, sheet_->surface(),
                -static_cast<double>(col * sheet_->frame_w()),
                -static_cast<double>(row * sheet_->frame_h()));
            cairo_rectangle(cr, 0, 0, sheet_->frame_w(), sheet_->frame_h());
            cairo_fill(cr);
            cairo_restore(cr);
            return;
        }
    }

    // Placeholder when no sprite is available: small filled circle so the
    // user can see the pet position and validate the cursor-chase loop.
    cairo_save(cr);
    cairo_set_source_rgba(cr, 0.95, 0.55, 0.15, 0.85);
    cairo_arc(cr, pet_.position.x, pet_.position.y, 12.0, 0, 6.28318);
    cairo_fill(cr);
    cairo_restore(cr);
    (void)w; (void)h;
}

std::string App::handle_request(const std::string& req) {
    using namespace std::chrono;
    if (req == "pet:toggle") { pet_visible_ = !pet_visible_; return pet_visible_ ? "visible" : "hidden"; }
    if (req == "pet:quit")   { overlay_.quit(); return "quitting"; }
    if (req == "pet:status") {
        auto& p = UserPersona::active();
        return std::string(p.greeting()) + " — pet="
             + (pet_visible_ ? "visible" : "hidden")
             + " state=" + std::string(PetStateMachine::state_name(pet_.fsm.state()));
    }
    if (req == "pomodoro:start")  { pomodoro_.start();  return "ok"; }
    if (req == "pomodoro:pause")  { pomodoro_.pause();  return "ok"; }
    if (req == "pomodoro:resume") { pomodoro_.resume(); return "ok"; }
    if (req == "pomodoro:stop")   { pomodoro_.stop();   return "ok"; }
    if (req == "pomodoro:skip")   { pomodoro_.skip();   return "ok"; }
    if (req == "pomodoro:toggle") { pomodoro_.toggle(); return "ok"; }
    if (req == "pomodoro:status") {
        const auto rem = pomodoro_.remaining(PomodoroTimer::Clock::now());
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%s %02ld:%02ld",
            std::string(PomodoroTimer::phase_name(pomodoro_.phase())).c_str(),
            rem.count() / 60, rem.count() % 60);
        return buf;
    }
    if (req == "waybar-status") return waybar_json(pomodoro_, pet_visible_);
    return "error: unknown request";
}

} // namespace hyprneko
