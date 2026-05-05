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
};

CliResult parse_cli(int argc, char** argv);

void print_help(std::ostream& os);
void print_version(std::ostream& os);

} // namespace aymm
