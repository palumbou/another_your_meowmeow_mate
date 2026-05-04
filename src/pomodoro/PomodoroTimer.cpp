#include "hyprneko/PomodoroTimer.hpp"

namespace hyprneko {

PomodoroTimer::PomodoroTimer(PomodoroConfig cfg) : cfg_(cfg) {}

std::string_view PomodoroTimer::phase_name(PomodoroPhase p) {
    switch (p) {
        case PomodoroPhase::Stopped:   return "stopped";
        case PomodoroPhase::Focus:     return "focus";
        case PomodoroPhase::Break:     return "break";
        case PomodoroPhase::LongBreak: return "long_break";
        case PomodoroPhase::Paused:    return "paused";
    }
    return "unknown";
}

std::chrono::seconds PomodoroTimer::phase_duration(PomodoroPhase p) const {
    using namespace std::chrono;
    switch (p) {
        case PomodoroPhase::Focus:     return minutes(cfg_.study_minutes);
        case PomodoroPhase::Break:     return minutes(cfg_.break_minutes);
        case PomodoroPhase::LongBreak: return minutes(cfg_.long_break_minutes);
        default:                       return seconds(0);
    }
}

void PomodoroTimer::enter(PomodoroPhase p, TimePoint now) {
    phase_       = p;
    phase_start_ = now;
}

void PomodoroTimer::start() {
    if (phase_ != PomodoroPhase::Stopped) return;
    enter(PomodoroPhase::Focus, Clock::now());
}

void PomodoroTimer::pause() {
    if (phase_ == PomodoroPhase::Stopped || phase_ == PomodoroPhase::Paused) return;
    paused_phase_ = phase_;
    pause_start_  = Clock::now();
    phase_        = PomodoroPhase::Paused;
}

void PomodoroTimer::resume() {
    if (phase_ != PomodoroPhase::Paused) return;
    const auto delta = Clock::now() - pause_start_;
    phase_start_    += delta;
    phase_           = paused_phase_;
    paused_phase_    = PomodoroPhase::Stopped;
}

void PomodoroTimer::stop() {
    phase_           = PomodoroPhase::Stopped;
    paused_phase_    = PomodoroPhase::Stopped;
    completed_focus_ = 0;
}

void PomodoroTimer::skip() {
    if (phase_ == PomodoroPhase::Stopped) return;
    if (phase_ == PomodoroPhase::Paused)  phase_ = paused_phase_;
    // Force-expire current phase by setting phase_start_ far in the past.
    phase_start_ = Clock::now() - std::chrono::hours(24);
    tick(Clock::now());
}

void PomodoroTimer::toggle() {
    switch (phase_) {
        case PomodoroPhase::Stopped: start();  break;
        case PomodoroPhase::Paused:  resume(); break;
        default:                     pause();  break;
    }
}

void PomodoroTimer::tick(TimePoint now) {
    if (phase_ == PomodoroPhase::Stopped || phase_ == PomodoroPhase::Paused) return;
    const auto elapsed = now - phase_start_;
    if (elapsed < phase_duration(phase_)) return;

    // Phase finished — transition.
    switch (phase_) {
        case PomodoroPhase::Focus: {
            ++completed_focus_;
            const bool long_due = (completed_focus_ % cfg_.sessions_before_long_break) == 0;
            enter(long_due ? PomodoroPhase::LongBreak : PomodoroPhase::Break, now);
            break;
        }
        case PomodoroPhase::Break:
        case PomodoroPhase::LongBreak:
            enter(PomodoroPhase::Focus, now);
            break;
        default: break;
    }
}

std::chrono::seconds PomodoroTimer::remaining(TimePoint now) const {
    using namespace std::chrono;
    if (phase_ == PomodoroPhase::Stopped) return seconds(0);
    const auto active = (phase_ == PomodoroPhase::Paused) ? paused_phase_ : phase_;
    const auto dur    = phase_duration(active);
    const auto ref    = (phase_ == PomodoroPhase::Paused) ? pause_start_ : now;
    const auto rem    = dur - duration_cast<seconds>(ref - phase_start_);
    return rem.count() > 0 ? rem : seconds(0);
}

int PomodoroTimer::progress_percent(TimePoint now) const {
    using namespace std::chrono;
    if (phase_ == PomodoroPhase::Stopped) return 0;
    const auto active = (phase_ == PomodoroPhase::Paused) ? paused_phase_ : phase_;
    const auto dur    = phase_duration(active);
    if (dur.count() == 0) return 0;
    const auto rem  = remaining(now);
    const auto done = dur - rem;
    return static_cast<int>(100.0 * done.count() / dur.count());
}

} // namespace hyprneko
