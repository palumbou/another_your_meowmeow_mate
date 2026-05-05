#pragma once

#include <string>
#include <string_view>

namespace aymm {

// Small abstraction for user-facing greetings/labels. Default impl is neutral.
// The `feature/queen-bianca-mode` branch overrides the active set when the
// system username matches a Bianca-variant.
class UserPersona {
public:
    virtual ~UserPersona() = default;

    static const UserPersona& active();

    // e.g. "Welcome back" / "Welcome back, Queen"
    std::string_view greeting()       const { return greeting_;       }
    // Waybar tooltip prefix, e.g. "Pomodoro" / "Queen's Pomodoro"
    std::string_view tooltip_prefix() const { return tooltip_prefix_; }
    // Notification body suffix, e.g. "" / ", Queen"
    std::string_view honorific()      const { return honorific_;      }

    // Builds the Pomodoro phase tooltip: "<prefix>: <phase> — <time> remaining"
    // by default. Personas may override to use a different shape (e.g. the
    // Queen persona uses a possessive form).
    virtual std::string format_phase_tooltip(std::string_view phase_label,
                                             std::string_view time_remaining,
                                             bool paused) const;

protected:
    UserPersona(std::string_view greeting,
                std::string_view tooltip_prefix,
                std::string_view honorific)
        : greeting_(greeting), tooltip_prefix_(tooltip_prefix), honorific_(honorific) {}

private:
    std::string_view greeting_;
    std::string_view tooltip_prefix_;
    std::string_view honorific_;
};

// Returns current Linux username (USER env or getpwuid). Useful for tests &
// for the queen-mode override on the feature branch.
std::string current_username();

} // namespace aymm
