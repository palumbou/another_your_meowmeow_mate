#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace aymm {

struct CliResult {
    enum class Action {
        RunDaemon,        // no subcommand or `run`: launch the pet
        ForwardToDaemon,  // send `request` to a running daemon
        PrintWaybarStatus,// `waybar-status`: read from daemon if available, else default
        PrintHelp,
        PrintVersion,
        Error,
    };
    Action action = Action::RunDaemon;
    std::string request;
    std::string error;
    std::string config_path;
    bool autostart_pomodoro = false;   // --pomodoro flag (only for RunDaemon)

    // Pomodoro overrides applied on top of the loaded config. -1 means
    // "leave the loaded value alone".
    int  study_min        = -1;
    int  break_min        = -1;
    int  long_break_min   = -1;
    int  sessions_before_long_break = -1;
    std::string focus_corner;          // empty = unchanged
};

CliResult parse_cli(int argc, char** argv);

void print_help(std::ostream& os);
void print_version(std::ostream& os);

} // namespace aymm
