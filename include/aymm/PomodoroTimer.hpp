#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

namespace aymm {

enum class PomodoroPhase {
    Stopped,   // not running
    Focus,     // study session
    Break,     // short break
    LongBreak,
    Paused,    // user-paused (whatever phase was active)
};

struct PomodoroConfig {
    int study_minutes              = 25;
    int break_minutes              = 5;
    int long_break_minutes         = 15;
    int sessions_before_long_break = 4;
    bool auto_start                = false;
    bool show_notifications        = true;
};

class PomodoroTimer {
public:
    using Clock     = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    explicit PomodoroTimer(PomodoroConfig cfg);

    void start();   // start a focus session if Stopped
    void pause();
    void resume();
    void stop();
    void skip();    // go to next phase immediately
    void toggle();  // start if stopped, pause/resume otherwise
    void tick(TimePoint now);

    PomodoroPhase phase() const { return phase_; }
    PomodoroPhase paused_phase() const { return paused_phase_; }
    int  completed_focus_sessions() const { return completed_focus_; }
    std::chrono::seconds remaining(TimePoint now) const;
    int  progress_percent(TimePoint now) const;

    static std::string_view phase_name(PomodoroPhase p);

    const PomodoroConfig& config() const { return cfg_; }

private:
    void enter(PomodoroPhase phase, TimePoint now);
    std::chrono::seconds phase_duration(PomodoroPhase p) const;

    PomodoroConfig cfg_;
    PomodoroPhase  phase_         = PomodoroPhase::Stopped;
    PomodoroPhase  paused_phase_  = PomodoroPhase::Stopped;
    TimePoint      phase_start_{};
    TimePoint      pause_start_{};
    int            completed_focus_ = 0;
};

} // namespace aymm
