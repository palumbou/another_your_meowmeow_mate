#include "hyprneko/Cli.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace hyprneko {

void print_help(std::ostream& os) {
    os <<
"hyprneko — Wayland-native desktop pet (Hyprland-first)\n"
"\n"
"Usage:\n"
"  hyprneko [run]                    Start the pet daemon (default).\n"
"  hyprneko toggle                   Show/hide the pet on a running daemon.\n"
"  hyprneko quit                     Tell the running daemon to exit.\n"
"  hyprneko status                   Print human-readable daemon status.\n"
"  hyprneko waybar-status            Print Waybar JSON (one shot).\n"
"  hyprneko pomodoro <cmd>           Pomodoro control. <cmd> is one of:\n"
"      start | pause | resume | stop | skip | toggle | status\n"
"\n"
"Options:\n"
"  --config <path>                   Override config file path.\n"
"  -h, --help                        Show this help.\n"
"  --version                         Print version and exit.\n";
}

void print_version(std::ostream& os) {
    os << "hyprneko " << HYPRNEKO_VERSION << "\n";
}

CliResult parse_cli(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    CliResult r;

    for (size_t i = 0; i < args.size(); ) {
        const auto& a = args[i];
        if (a == "-h" || a == "--help")      { r.action = CliResult::Action::PrintHelp;    return r; }
        if (a == "--version")                 { r.action = CliResult::Action::PrintVersion; return r; }
        if (a == "--config" && i + 1 < args.size()) {
            r.config_path = args[i + 1]; i += 2; continue;
        }
        if (a == "run")     { r.action = CliResult::Action::RunDaemon; ++i; continue; }
        if (a == "toggle")  { r.action = CliResult::Action::ForwardToDaemon;  r.request = "pet:toggle"; ++i; continue; }
        if (a == "quit")    { r.action = CliResult::Action::ForwardToDaemon;  r.request = "pet:quit";   ++i; continue; }
        if (a == "status")  { r.action = CliResult::Action::ForwardToDaemon;  r.request = "pet:status"; ++i; continue; }
        if (a == "waybar-status") {
            r.action = CliResult::Action::PrintWaybarStatus; ++i; continue;
        }
        if (a == "pomodoro" && i + 1 < args.size()) {
            const auto& sub = args[i + 1];
            r.action  = CliResult::Action::ForwardToDaemon;
            r.request = "pomodoro:" + sub;
            i += 2;
            continue;
        }
        r.action = CliResult::Action::Error;
        r.error  = "unknown argument: " + a;
        return r;
    }
    return r;
}

} // namespace hyprneko
