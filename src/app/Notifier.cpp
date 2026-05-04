#include "hyprneko/Notifier.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <spawn.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace hyprneko {

void Notifier::notify(std::string_view title,
                      std::string_view body,
                      std::string_view urgency) {
    if (!enabled_) return;

    const std::string title_s   { title };
    const std::string body_s    { body };
    const std::string urgency_s { urgency };

    // notify-send -u <urgency> -a hyprneko -- <title> <body>
    char* const argv[] = {
        const_cast<char*>("notify-send"),
        const_cast<char*>("-u"), const_cast<char*>(urgency_s.c_str()),
        const_cast<char*>("-a"), const_cast<char*>("hyprneko"),
        const_cast<char*>("--"),
        const_cast<char*>(title_s.c_str()),
        const_cast<char*>(body_s.c_str()),
        nullptr,
    };

    pid_t pid = 0;
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    // Don't inherit our SIGCHLD handler etc. — reap with WNOHANG below.

    const int rc = ::posix_spawnp(&pid, "notify-send", nullptr, &attr, argv, environ);
    posix_spawnattr_destroy(&attr);

    if (rc != 0) {
        if (!warned_once_) {
            warned_once_ = true;
            std::cerr << "hyprneko: notify-send not available (" << ::strerror(rc)
                      << "); notifications disabled until restart.\n";
            enabled_ = false;
        }
        return;
    }

    // Reap any previously spawned children non-blockingly so we don't pile up
    // zombies. notify-send exits quickly.
    int status = 0;
    while (::waitpid(-1, &status, WNOHANG) > 0) { /* drain */ }
}

} // namespace hyprneko
