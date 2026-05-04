#include "hyprneko/UserPersona.hpp"

#include <cstdlib>
#include <pwd.h>
#include <unistd.h>

namespace hyprneko {

namespace {

class NeutralPersona : public UserPersona {
public:
    NeutralPersona() : UserPersona("Welcome back", "Pomodoro", "") {}
};

#ifdef HYPRNEKO_QUEEN_MODE
// Case-insensitive match of common Bianca variants. Kept tiny on purpose so
// false positives stay rare (we don't want to greet random users as Queen).
bool is_bianca(std::string_view name) {
    auto lower = [](std::string s) {
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };
    const std::string n = lower(std::string(name));
    return n == "bianca" || n == "bianchina" || n == "queenbianca";
}

class QueenPersona : public UserPersona {
public:
    QueenPersona() : UserPersona("Welcome back, Queen", "Queen's Pomodoro", ", Queen") {}
};
#endif

} // namespace

std::string current_username() {
    if (const char* u = std::getenv("USER"); u && *u) return u;
    if (auto* pw = ::getpwuid(::getuid()); pw && pw->pw_name) return pw->pw_name;
    return {};
}

const UserPersona& UserPersona::active() {
#ifdef HYPRNEKO_QUEEN_MODE
    static const QueenPersona  queen;
    static const NeutralPersona neutral;
    return is_bianca(current_username()) ? static_cast<const UserPersona&>(queen)
                                         : static_cast<const UserPersona&>(neutral);
#else
    static const NeutralPersona neutral;
    return neutral;
#endif
}

} // namespace hyprneko
