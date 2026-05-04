#pragma once

#include <string>
#include <string_view>

namespace hyprneko {

// Fire-and-forget desktop notifications. Currently shells out to `notify-send`
// (via posix_spawn so we don't block the render loop on fork). If
// `notify-send` is not on PATH, the call is a no-op and we log a warning the
// first time only.
//
// Privacy: we never include cursor positions, window titles, file paths, or
// any keystroke content in the notification body — only Pomodoro phase.
class Notifier {
public:
    // urgency: "low" | "normal" | "critical"
    void notify(std::string_view title,
                std::string_view body,
                std::string_view urgency = "normal");

    bool enabled() const { return enabled_; }
    void set_enabled(bool e) { enabled_ = e; }

private:
    bool enabled_     = true;
    bool warned_once_ = false;
};

} // namespace hyprneko
