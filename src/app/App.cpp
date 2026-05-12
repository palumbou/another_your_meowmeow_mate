#include "aymm/App.hpp"
#include "aymm/ProceduralCat.hpp"
#include "aymm/UserPersona.hpp"
#include "aymm/WaybarStatus.hpp"

#include <cairo/cairo.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <sys/timerfd.h>
#include <thread>
#include <unistd.h>

namespace aymm {

namespace {

bool hyprland_session_detected() {
    const char* sig = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    return sig && *sig;
}

bool kde_session_detected() {
    const char* xdg = std::getenv("XDG_CURRENT_DESKTOP");
    if (xdg && std::strstr(xdg, "KDE")) return true;
    const char* full = std::getenv("KDE_FULL_SESSION");
    if (full && *full) return true;
    return false;
}

struct CursorPick {
    std::unique_ptr<CursorProvider> provider;
    EvdevCursorProvider*            evdev = nullptr;  // non-owning view if applicable
};

CursorPick make_cursor_provider(CursorSource s) {
    auto try_hyprland = []() -> std::unique_ptr<CursorProvider> {
        auto p = std::make_unique<HyprlandCursorProvider>();
        if (p->ready()) return p;
        return nullptr;
    };
    auto try_evdev = []() -> std::pair<std::unique_ptr<CursorProvider>, EvdevCursorProvider*> {
        auto p = std::make_unique<EvdevCursorProvider>();
        EvdevCursorProvider* raw = p.get();
        if (p->ready()) return { std::move(p), raw };
        return { nullptr, nullptr };
    };

    switch (s) {
        case CursorSource::Auto: {
            if (hyprland_session_detected()) {
                if (auto h = try_hyprland()) {
                    std::cerr << "aymm: cursor provider = hyprland (auto-detected)\n";
                    return { std::move(h), nullptr };
                }
                std::cerr << "aymm: HYPRLAND_INSTANCE_SIGNATURE set but socket not "
                             "reachable; trying evdev fallback.\n";
            } else {
                std::cerr << "aymm: not on Hyprland; trying evdev cursor provider "
                             "(requires the `input` group).\n";
            }
            auto [ev, raw] = try_evdev();
            if (ev) {
                if (kde_session_detected()) {
                    std::cerr << "aymm: KDE Plasma session detected. Heads-up: KWin uses "
                                 "libinput which grabs /dev/input/event* exclusively "
                                 "(EVIOCGRAB), so aymm's evdev reads return no events. "
                                 "The cat will render but cursor chase will not work. "
                                 "There is no public KDE API for cursor position. For a "
                                 "working chase loop on Fedora, log into a Hyprland "
                                 "session instead (`sudo dnf install hyprland`).\n";
                }
                return { std::move(ev), raw };
            }
            std::cerr << "aymm: no cursor provider available; pet will sit still.\n";
            return { std::make_unique<NullCursorProvider>(), nullptr };
        }
        case CursorSource::Hyprland: {
            if (auto h = try_hyprland()) return { std::move(h), nullptr };
            std::cerr << "aymm: Hyprland cursor socket not detected; "
                         "pass cursor_source=auto or cursor_source=evdev.\n";
            return { std::make_unique<NullCursorProvider>(), nullptr };
        }
        case CursorSource::Evdev: {
            auto [ev, raw] = try_evdev();
            if (ev) return { std::move(ev), raw };
            return { std::make_unique<NullCursorProvider>(), nullptr };
        }
        case CursorSource::Wlroots:
        case CursorSource::Null:
        default:
            return { std::make_unique<NullCursorProvider>(), nullptr };
    }
}

} // namespace

App::App(Config cfg, AppOptions opts)
  : cfg_(std::move(cfg)),
    opts_(opts),
    pomodoro_(PomodoroConfig{
        cfg_.pomodoro_study_minutes,
        cfg_.pomodoro_break_minutes,
        cfg_.pomodoro_long_break_minutes,
        cfg_.pomodoro_sessions_before_long_break,
        cfg_.pomodoro_auto_start,
        cfg_.pomodoro_show_notifications}),
    pomodoro_behavior_(pet_, pomodoro_, cfg_.movement_speed, cfg_.idle_distance)
{
    auto pick = make_cursor_provider(cfg_.cursor_source);
    cursor_ = std::move(pick.provider);
    evdev_  = pick.evdev;
    chase_  = std::make_unique<CursorChaseBehavior>(pet_, *cursor_);

    pet_.speed         = cfg_.movement_speed;
    pet_.idle_distance = cfg_.idle_distance;

    if (!cfg_.sprite_dir.empty()) {
        std::string err;
        sheet_ = SpriteSheet::load(cfg_.sprite_dir, err);
        if (sheet_) {
            animations_ = std::make_unique<AnimationResolver>(*sheet_);
        } else {
            std::cerr << "aymm: sprite sheet load failed: " << err
                      << "  (drawing the procedural cat instead).\n";
        }
    }

    notifier_.set_enabled(cfg_.pomodoro_show_notifications);

    // Pomodoro auto-start: either the config flag is on, or --pomodoro
    // was passed on the command line.
    if (opts_.autostart_pomodoro || (cfg_.enable_pomodoro && cfg_.pomodoro_auto_start)) {
        pomodoro_.start();
    }
    const bool starting_in_focus = (pomodoro_.phase() == PomodoroPhase::Focus);

    if (starting_in_focus) {
        // The cat is going straight into the basket to sleep — shouting
        // "Welcome back" while it's curling up doesn't make sense. Fire the
        // Focus phase-change handler instead so the user gets the proper
        // "Focus session started" notification + bubble.
        on_pomodoro_phase_changed(PomodoroPhase::Stopped, PomodoroPhase::Focus);
    } else {
        // Normal startup greeting. Both notify-send (system tray) and the
        // on-screen speech bubble (the cat tells you hello). Persona just
        // decides whether ", Queen" is appended.
        const auto& persona = UserPersona::active();
        const std::string greet = std::string(persona.greeting()) + "!";
        notifier_.notify("aymm", "🐈 " + greet, "low");
        say(greet);
    }

    last_pomodoro_phase_ = pomodoro_.phase();
}

void App::say(std::string_view text, std::chrono::milliseconds duration,
              SpeechBubble::Style style) {
    bubble_.say(text, duration, style);
}

int App::run() {
    std::string err;
    if (!overlay_.connect(cfg_, err)) {
        std::cerr << "aymm: " << err << "\n";
        return 1;
    }

    const Rect bounds  = overlay_.compositor_bounds();
    const Rect primary = overlay_.primary_output_rect();

    pet_.position.x = primary.x + primary.w / 2.0;
    pet_.position.y = primary.y + primary.h / 2.0;

    // If Pomodoro auto-started in the constructor (--pomodoro flag or config
    // auto-start), on_tick will see Focus phase and start walking the cat
    // toward the configured corner. No teleport — the cat takes itself to
    // the basket.

    // Hand the evdev provider its clamping rectangle and a sensible initial
    // cursor position. Hyprland provider doesn't need this (it reports
    // absolute compositor coordinates).
    if (evdev_) {
        evdev_->set_bounds(bounds.x, bounds.y, bounds.w, bounds.h);
        evdev_->set_position(static_cast<int>(pet_.position.x),
                             static_cast<int>(pet_.position.y));
    }

    overlay_.set_draw([this](cairo_t* cr, int w, int h, int ox, int oy){
        draw(cr, w, h, ox, oy);
    });

    // BTN_RIGHT = 0x111 — right-click on the cat cycles Ball/Food/Purr.
    // BTN_LEFT and BTN_MIDDLE are ignored for now.
    overlay_.set_button_callback([this](uint32_t button, double gx, double gy){
        if (button != 0x111) return;
        // Sanity-check that the click is reasonably close to the cat (the
        // input region is also throttled, so this is a belt-and-suspenders).
        const double dx = gx - pet_.position.x;
        const double dy = gy - pet_.position.y;
        if (dx * dx + dy * dy > 100.0 * 100.0) return;
        const Direction d = pet_.fsm.direction();
        const bool facing_east = (d == Direction::None)
                              || (d == Direction::E) || (d == Direction::NE) || (d == Direction::SE);
        action_.trigger_next(ActionState::Clock::now(),
                             pet_.position.x, pet_.position.y, facing_east);
    });

    if (control_.bind_listen() < 0) {
        std::cerr << "aymm: control socket bind failed (errno="
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
    const auto now = PomodoroTimer::Clock::now();
    pomodoro_.tick(now);
    if (pomodoro_.phase() != last_pomodoro_phase_) {
        on_pomodoro_phase_changed(last_pomodoro_phase_, pomodoro_.phase());
        last_pomodoro_phase_ = pomodoro_.phase();
    }
    pomodoro_behavior_.apply();
    action_.update(now);

    // Update the input region around the cat so right-clicks on it land in
    // our surface. Throttle: only push to the compositor when the cat has
    // moved more than 10 px since the last update.
    const int cgx = static_cast<int>(pet_.position.x);
    const int cgy = static_cast<int>(pet_.position.y);
    if (std::abs(cgx - last_region_gx_) > 10 || std::abs(cgy - last_region_gy_) > 10) {
        overlay_.set_cat_region(cgx, cgy, 110);
        last_region_gx_ = cgx;
        last_region_gy_ = cgy;
    }

    // Active right-click action takes precedence over chase / pomodoro.
    if (action_.active()) {
        switch (action_.current()) {
            case ActionState::Effect::Ball: {
                // Chase the yarn ball but stop ~40 px short so the ball
                // doesn't end up under the cat's belly.
                const double dx = action_.prop_x - pet_.position.x;
                const double dy = action_.prop_y - pet_.position.y;
                const double dist = std::sqrt(dx * dx + dy * dy);
                if (dist <= 40.0) {
                    pet_.has_target = false;
                    if (pet_.fsm.state() == PetState::Run) {
                        pet_.fsm.set_state(PetState::Idle, now);
                    }
                } else {
                    pet_.target.x = action_.prop_x;
                    pet_.target.y = action_.prop_y;
                    pet_.has_target = true;
                }
                pet_.step(now);
                return;
            }
            case ActionState::Effect::Food:
            case ActionState::Effect::Purr:
            case ActionState::Effect::Scratch:
                // Stay put; rendered visuals go on top in draw().
                pet_.has_target = false;
                if (pet_.fsm.state() != PetState::Idle) {
                    pet_.fsm.set_state(PetState::Idle, now);
                }
                pet_.step(now);
                return;
            default: break;
        }
    }

    // Pomodoro gate: any time a Pomodoro session is running, the cat behavior
    // is dictated by the phase. The config-level `enable_pomodoro` flag only
    // affects whether the timer auto-starts; it does NOT gate the behavior.
    bool should_chase = (cfg_.behavior == Behavior::CursorChase);
    if (pomodoro_.phase() != PomodoroPhase::Stopped) {
        const auto active = (pomodoro_.phase() == PomodoroPhase::Paused)
                          ? pomodoro_.paused_phase() : pomodoro_.phase();
        if (active == PomodoroPhase::Focus) {
            // Walk the cat toward the configured focus corner using the same
            // mechanics as cursor chase. When close enough, settle into Sleep.
            // No teleport — at the end of a Break the cat runs back to its
            // basket on its own paws.
            const Vec2 corner = focus_corner_position();
            const double dx = corner.x - pet_.position.x;
            const double dy = corner.y - pet_.position.y;
            const double dist = std::sqrt(dx * dx + dy * dy);
            if (dist <= pet_.idle_distance) {
                if (pet_.fsm.state() != PetState::Sleep) {
                    pet_.fsm.set_state(PetState::Sleep, now);
                }
                pet_.has_target = false;
            } else {
                pet_.target = corner;
                pet_.has_target = true;
            }
            pet_.step(now);
            return;
        }
    }

    if (should_chase) {
        chase_->update(now);
    } else {
        pet_.has_target = false;
        pet_.step(now);
    }
}

Vec2 App::focus_corner_position() const {
    const Rect r = overlay_.primary_output_rect();
    if (r.w <= 0 || r.h <= 0) return { pet_.position.x, pet_.position.y };
    const int pad = std::max(40, cfg_.pomodoro_focus_padding);
    switch (cfg_.pomodoro_focus_corner) {
        case FocusCorner::None:        return { pet_.position.x, pet_.position.y };
        case FocusCorner::Center:      return { r.x + r.w / 2.0,       r.y + r.h / 2.0       };
        case FocusCorner::TopLeft:     return { double(r.x + pad),     double(r.y + pad)     };
        case FocusCorner::TopRight:    return { double(r.x + r.w - pad), double(r.y + pad)   };
        case FocusCorner::BottomLeft:  return { double(r.x + pad),     double(r.y + r.h - pad) };
        case FocusCorner::BottomRight:
        default:                       return { double(r.x + r.w - pad), double(r.y + r.h - pad) };
    }
}

void App::on_pomodoro_phase_changed(PomodoroPhase from, PomodoroPhase to) {
    const auto& persona = UserPersona::active();
    const std::string honor { persona.honorific() };

    // Notifications and bubbles can both carry emoji now: the bubble goes
    // through Pango, the notification daemon uses the system font stack
    // — both end up at Noto Color Emoji or whatever's installed. The
    // notification body is prefixed with 🐈 so the popup reads obviously
    // as coming from aymm.
    auto announce = [&](std::string_view body, std::string_view bubble,
                        std::string_view urgency,
                        SpeechBubble::Style style = SpeechBubble::Style::Speech) {
        if (cfg_.pomodoro_show_notifications) {
            const std::string with_icon = "🐈 " + std::string(body);
            notifier_.notify("aymm", with_icon, urgency);
        }
        say(bubble, std::chrono::milliseconds(4500), style);
    };

    // Entering Focus does NOT teleport the cat anymore — on_tick walks it
    // toward focus_corner_position() and only sets PetState::Sleep when it
    // gets close enough. We use Status (no tail) bubble style during focus
    // because the cat is going to sleep — a speech bubble would imply it's
    // talking while curled up in the basket.

    switch (to) {
        case PomodoroPhase::Focus:
            if (from == PomodoroPhase::Stopped) {
                announce("Focus session started" + honor + ".",
                         "💤 Focus time" + honor,
                         "low",
                         SpeechBubble::Style::Status);
            } else {
                announce("Back to focus" + honor + ". You've got this.",
                         "💤 Back to focus" + honor,
                         "low",
                         SpeechBubble::Style::Status);
            }
            break;
        case PomodoroPhase::Break:
            announce("Your break awaits" + honor + ".",
                     "Break time" + honor + " 🐾",
                     "normal");
            break;
        case PomodoroPhase::LongBreak:
            announce("Long break time" + honor + " — go stretch.",
                     "Long break" + honor + " 🐾",
                     "normal");
            break;
        default: break;
    }
}

void App::draw(cairo_t* cr, int w, int h, int origin_x, int origin_y) {
    if (!pet_visible_) return;
    cairo_save(cr);
    cairo_translate(cr, -origin_x, -origin_y);

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
        }
    } else {
        // No PNG sprite sheet — fall back to the built-in procedural cat at
        // 4.0× scale (cat is ~80×60 px on a 1080p monitor).
        draw_procedural_cat(cr, pet_.position.x, pet_.position.y,
                            4.0,
                            pet_.fsm.state(), pet_.fsm.direction(),
                            pet_.fsm.frame());
    }

    // Interactive action visuals (ball, food bowl, purring hearts) sit on
    // top of the cat but below the speech bubble.
    if (action_.active()) draw_action(cr);

    // Speech bubble on top of everything else.
    if (bubble_.active()) {
        bubble_.draw(cr, pet_.position.x, pet_.position.y);
    }

    cairo_restore(cr);
    (void)w; (void)h;
}

std::string App::handle_request(const std::string& req) {
    using namespace std::chrono;
    const auto& persona = UserPersona::active();

    if (req == "pet:toggle") {
        pet_visible_ = !pet_visible_;
        return pet_visible_ ? "visible" : "hidden";
    }
    if (req == "pet:quit") {
        const std::string bye = "Goodbye" + std::string(persona.honorific()) + "!";
        notifier_.notify("aymm", "🐈 " + bye, "low");
        say(bye, std::chrono::milliseconds(2500));
        // Defer the actual quit so the bubble has a chance to render.
        // Two redraws at 30 Hz is ~67 ms; we give 2 s to be safe.
        std::thread([this](){
            std::this_thread::sleep_for(std::chrono::seconds(2));
            overlay_.quit();
        }).detach();
        return "quitting";
    }
    if (req == "pet:status") {
        return std::string(persona.greeting()) + " — pet="
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

void App::draw_action(cairo_t* cr) {
    const auto now = ActionState::Clock::now();
    const double p = action_.progress(now);
    constexpr double kPi = 3.14159265358979323846;

    cairo_save(cr);
    switch (action_.current()) {
        case ActionState::Effect::Ball: {
            // Yarn ball — cream-colored sphere with thin lines wrapping
            // around it to suggest a wound thread.
            const double bx = action_.prop_x;
            const double by = action_.prop_y;
            // Drop shadow
            cairo_set_source_rgba(cr, 0, 0, 0, 0.25);
            cairo_save(cr);
            cairo_translate(cr, bx, by + 11);
            cairo_scale(cr, 1.0, 0.35);
            cairo_arc(cr, 0, 0, 12, 0, 2 * kPi);
            cairo_restore(cr);
            cairo_fill(cr);
            // Yarn body
            cairo_set_source_rgba(cr, 0.96, 0.85, 0.62, 1.0);
            cairo_arc(cr, bx, by, 12, 0, 2 * kPi);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, 0.55, 0.40, 0.20, 0.85);
            cairo_set_line_width(cr, 0.8);
            cairo_stroke(cr);
            // Wrapping threads — a few curved strokes across the ball.
            cairo_set_source_rgba(cr, 0.70, 0.50, 0.25, 0.85);
            cairo_set_line_width(cr, 0.7);
            // Clip strokes to ball circle.
            cairo_save(cr);
            cairo_arc(cr, bx, by, 11.5, 0, 2 * kPi);
            cairo_clip(cr);
            for (int i = -2; i <= 2; ++i) {
                cairo_save(cr);
                cairo_translate(cr, bx, by);
                cairo_rotate(cr, i * 0.45);
                cairo_move_to(cr, -14, -3 + i * 1.2);
                cairo_curve_to(cr, -4, 0 + i, 4, 1 + i, 14, -3 + i * 1.2);
                cairo_stroke(cr);
                cairo_restore(cr);
            }
            for (int i = -2; i <= 2; ++i) {
                cairo_save(cr);
                cairo_translate(cr, bx, by);
                cairo_rotate(cr, kPi / 2 + i * 0.45);
                cairo_move_to(cr, -14, -3 + i * 1.2);
                cairo_curve_to(cr, -4, 0 + i, 4, 1 + i, 14, -3 + i * 1.2);
                cairo_stroke(cr);
                cairo_restore(cr);
            }
            cairo_restore(cr);
            // Tiny loose end coming off the ball.
            cairo_set_source_rgba(cr, 0.70, 0.50, 0.25, 0.95);
            cairo_set_line_width(cr, 1.1);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_move_to(cr, bx + 11, by - 6);
            cairo_curve_to(cr, bx + 17, by - 10, bx + 19, by - 4, bx + 16, by + 1);
            cairo_stroke(cr);
            break;
        }
        case ActionState::Effect::Food: {
            // Brown bowl positioned in front of the cat, heaped with food.
            const double bx = action_.prop_x;
            const double by = action_.prop_y;
            // Drop shadow
            cairo_set_source_rgba(cr, 0, 0, 0, 0.25);
            cairo_save(cr);
            cairo_translate(cr, bx, by + 6);
            cairo_scale(cr, 1.0, 0.35);
            cairo_arc(cr, 0, 0, 24, 0, 2 * kPi);
            cairo_restore(cr);
            cairo_fill(cr);
            // Bowl back rim (top of bowl, behind food)
            cairo_set_source_rgba(cr, 0.42, 0.26, 0.12, 1.0);
            cairo_save(cr);
            cairo_translate(cr, bx, by - 3);
            cairo_scale(cr, 1.0, 0.4);
            cairo_arc(cr, 0, 0, 22, kPi, 2 * kPi);
            cairo_restore(cr);
            cairo_fill(cr);
            // Heaped food: a dome of chunks above the bowl rim.
            const double food_colors[3][3] = {
                { 0.78, 0.50, 0.22 },
                { 0.85, 0.58, 0.28 },
                { 0.62, 0.36, 0.16 },
            };
            // Sketch a mound of food using overlapping circles.
            const struct { double dx, dy, r; int c; } chunks[] = {
                {  -14,  -3, 3.0, 0 }, {  -9,  -5, 3.2, 1 }, {  -4,  -6, 3.0, 2 },
                {    1,  -7, 3.4, 0 }, {   6,  -6, 3.1, 1 }, {  10,  -4, 3.0, 2 },
                {   14,  -3, 2.8, 0 }, {  -11,   0, 3.1, 2 }, {  -5,  -2, 3.0, 1 },
                {    0,  -3, 3.0, 0 }, {   5,  -2, 2.9, 2 }, {  11,   0, 3.0, 1 },
            };
            for (auto& c : chunks) {
                cairo_set_source_rgba(cr, food_colors[c.c][0], food_colors[c.c][1],
                                      food_colors[c.c][2], 1.0);
                cairo_arc(cr, bx + c.dx, by + c.dy, c.r, 0, 2 * kPi);
                cairo_fill(cr);
            }
            // Bowl front rim (in front of food, with rim shine)
            cairo_pattern_t* g = cairo_pattern_create_linear(0, by, 0, by + 10);
            cairo_pattern_add_color_stop_rgb(g, 0.0, 0.72, 0.48, 0.20);
            cairo_pattern_add_color_stop_rgb(g, 1.0, 0.40, 0.24, 0.10);
            cairo_save(cr);
            cairo_translate(cr, bx, by);
            cairo_scale(cr, 1.0, 0.55);
            cairo_arc(cr, 0, 0, 22, 0, kPi);
            cairo_restore(cr);
            cairo_set_source(cr, g);
            cairo_fill_preserve(cr);
            cairo_pattern_destroy(g);
            cairo_set_source_rgba(cr, 0.28, 0.18, 0.08, 0.9);
            cairo_set_line_width(cr, 0.8);
            cairo_stroke(cr);
            // Rim highlight on the front lip
            cairo_set_source_rgba(cr, 0.95, 0.85, 0.65, 0.5);
            cairo_set_line_width(cr, 0.5);
            cairo_save(cr);
            cairo_translate(cr, bx, by);
            cairo_scale(cr, 1.0, 0.55);
            cairo_arc(cr, 0, 0, 21, 0.2, kPi - 0.2);
            cairo_restore(cr);
            cairo_stroke(cr);
            break;
        }
        case ActionState::Effect::Scratch: {
            // Wooden scratching post with sisal-rope wrap. Vertical column
            // standing on a square base, set in front of the cat.
            const double px = action_.prop_x;
            const double py = action_.prop_y;
            // Base (rectangular)
            cairo_set_source_rgba(cr, 0, 0, 0, 0.25);
            cairo_rectangle(cr, px - 14, py + 26, 28, 4);
            cairo_fill(cr);
            cairo_set_source_rgba(cr, 0.45, 0.28, 0.15, 1.0);
            cairo_rectangle(cr, px - 14, py + 20, 28, 8);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, 0.25, 0.15, 0.06, 0.9);
            cairo_set_line_width(cr, 0.8);
            cairo_stroke(cr);
            // Post column wrapped in sisal — beige cylinder with horizontal
            // stripes for the rope coils.
            const double post_top = py - 35;
            const double post_bot = py + 20;
            const double post_w = 12;
            cairo_set_source_rgba(cr, 0.85, 0.68, 0.40, 1.0);
            cairo_rectangle(cr, px - post_w / 2, post_top, post_w, post_bot - post_top);
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, 0.55, 0.40, 0.20, 0.9);
            cairo_set_line_width(cr, 0.8);
            cairo_stroke(cr);
            // Horizontal rope coils across the post.
            cairo_set_source_rgba(cr, 0.55, 0.40, 0.20, 0.7);
            cairo_set_line_width(cr, 0.55);
            for (double y = post_top + 2; y < post_bot - 1; y += 3) {
                cairo_move_to(cr, px - post_w / 2 + 0.5, y);
                cairo_line_to(cr, px + post_w / 2 - 0.5, y);
                cairo_stroke(cr);
            }
            // Top cap — small flat disc.
            cairo_set_source_rgba(cr, 0.55, 0.35, 0.18, 1.0);
            cairo_save(cr);
            cairo_translate(cr, px, post_top);
            cairo_scale(cr, 1.0, 0.3);
            cairo_arc(cr, 0, 0, post_w / 2 + 2, 0, 2 * kPi);
            cairo_restore(cr);
            cairo_fill(cr);
            // Tiny claw marks on the post (animate position with progress).
            const double mark_y = post_top + 10 + p * 30.0;
            cairo_set_source_rgba(cr, 0.40, 0.25, 0.10, 0.9);
            cairo_set_line_width(cr, 0.7);
            cairo_move_to(cr, px - 4, mark_y);     cairo_line_to(cr, px - 1, mark_y + 6);
            cairo_move_to(cr, px,     mark_y - 1); cairo_line_to(cr, px + 3, mark_y + 5);
            cairo_move_to(cr, px + 4, mark_y);     cairo_line_to(cr, px + 7, mark_y + 6);
            cairo_stroke(cr);
            break;
        }
        case ActionState::Effect::Purr: {
            // Three pink hearts floating up + slightly to the side.
            auto path_heart = [&](double cx, double cy, double r) {
                cairo_new_sub_path(cr);
                cairo_arc(cr, cx - r * 0.55, cy - r * 0.30, r * 0.65, 0, 2 * kPi);
                cairo_new_sub_path(cr);
                cairo_arc(cr, cx + r * 0.55, cy - r * 0.30, r * 0.65, 0, 2 * kPi);
                cairo_new_sub_path(cr);
                cairo_move_to(cr, cx - r * 0.95, cy);
                cairo_line_to(cr, cx + r * 0.95, cy);
                cairo_line_to(cr, cx,            cy + r * 1.25);
                cairo_close_path(cr);
            };
            for (int i = 0; i < 3; ++i) {
                const double phase = p + i * 0.33;
                const double t = phase - std::floor(phase);  // 0..1 each cycle
                const double drift_x = std::sin((p + i) * 2.5) * 12.0 + (i - 1) * 8.0;
                const double y_off   = -20.0 - t * 50.0;
                const double a       = std::max(0.0, 1.0 - t);
                const double size    = 4.0 + t * 1.5;
                cairo_set_source_rgba(cr, 0.94, 0.40, 0.55, a);
                path_heart(pet_.position.x + drift_x, pet_.position.y + y_off, size);
                cairo_fill(cr);
            }
            break;
        }
        default: break;
    }
    cairo_restore(cr);
    (void)p;
}

} // namespace aymm
