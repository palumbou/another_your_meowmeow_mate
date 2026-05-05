#include "aymm/UserPersona.hpp"

#include <cctype>
#include <cstdlib>
#include <pwd.h>
#include <string>
#include <unistd.h>

namespace aymm {

namespace {

class NeutralPersona : public UserPersona {
public:
    NeutralPersona() : UserPersona("Welcome back", "Pomodoro", "") {}
};

#ifdef AYMM_QUEEN_MODE
// Strict, case-insensitive match. We deliberately accept only the exact
// username "bianca" or its English equivalent "white" — no nicknames or
// affixed variants — to keep the activation surface tiny and to avoid
// greeting an unrelated user as Queen by accident.
bool is_bianca(std::string_view name) {
    auto lower = [](std::string s) {
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };
    const std::string n = lower(std::string(name));
    return n == "bianca" || n == "white";
}

class QueenPersona : public UserPersona {
public:
    QueenPersona() : UserPersona("Welcome back, Queen", "Queen's Pomodoro", ", Queen") {}

    // Override the tooltip shape to use the possessive form requested in the
    // spec: "Queen's focus session — 24:12 remaining".
    std::string format_phase_tooltip(std::string_view phase_label,
                                     std::string_view time_remaining,
                                     bool paused) const override {
        std::string out;
        out.reserve(phase_label.size() + time_remaining.size() + 32);
        out.append("Queen's ").append(phase_label);
        if (paused) out.append(" (paused)");
        out.append(" — ").append(time_remaining).append(" remaining");
        return out;
    }
};
#endif

} // namespace

std::string current_username() {
    if (const char* u = std::getenv("USER"); u && *u) return u;
    if (auto* pw = ::getpwuid(::getuid()); pw && pw->pw_name) return pw->pw_name;
    return {};
}

std::string UserPersona::format_phase_tooltip(std::string_view phase_label,
                                              std::string_view time_remaining,
                                              bool paused) const {
    std::string out;
    out.reserve(tooltip_prefix_.size() + phase_label.size() + time_remaining.size() + 32);
    out.append(tooltip_prefix_).append(": ").append(phase_label);
    if (paused) out.append(" (paused)");
    out.append(" — ").append(time_remaining).append(" remaining");
    return out;
}

const UserPersona& UserPersona::active() {
#ifdef AYMM_QUEEN_MODE
    static const QueenPersona  queen;
    static const NeutralPersona neutral;
    return is_bianca(current_username()) ? static_cast<const UserPersona&>(queen)
                                         : static_cast<const UserPersona&>(neutral);
#else
    static const NeutralPersona neutral;
    return neutral;
#endif
}

} // namespace aymm
