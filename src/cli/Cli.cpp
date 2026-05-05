#include "aymm/Cli.hpp"

#include <iostream>
#include <string>
#include <vector>

namespace aymm {

void print_help(std::ostream& os) {
    os <<
"aymm — Wayland-native desktop pet (Hyprland-first)\n"
"\n"
"Usage:\n"
"  aymm [run]                    Start the pet daemon (default).\n"
"  aymm --pomodoro               Start the daemon and immediately begin a\n"
"                                Pomodoro session — one command, no daemon\n"
"                                needed up front.\n"
"  aymm toggle                   Show/hide the pet on a running daemon.\n"
"  aymm quit                     Tell the running daemon to exit (cat says\n"
"                                goodbye on screen and via notify-send).\n"
"  aymm status                   Print human-readable daemon status.\n"
"  aymm waybar-status            Print Waybar JSON (one shot).\n"
"  aymm pomodoro <cmd>           Pomodoro control on a running daemon.\n"
"                                <cmd>: start | pause | resume | stop\n"
"                                       | skip | toggle | status\n"
"\n"
"Options:\n"
"  -c, --config <path>           Use the config file at <path> instead of\n"
"                                $XDG_CONFIG_HOME/aymm/aymm.conf. Both forms\n"
"                                accepted: '--config path' and '--config=path'.\n"
"                                You can point this directly at the shipped\n"
"                                example without copying first, e.g.\n"
"                                  aymm -c examples/aymm.conf\n"
"  --pomodoro                    Auto-start a Pomodoro session at launch.\n"
"  -h, --help                    Show this help.\n"
"  --version                     Print version and exit.\n";
}

void print_version(std::ostream& os) {
    os << "aymm " << AYMM_VERSION << "\n";
}

CliResult parse_cli(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    CliResult r;

    for (size_t i = 0; i < args.size(); ) {
        const auto& a = args[i];
        if (a == "-h" || a == "--help")    { r.action = CliResult::Action::PrintHelp;    return r; }
        if (a == "--version")              { r.action = CliResult::Action::PrintVersion; return r; }
        // Config file path. Accept all four forms: '--config X', '--config=X',
        // '-c X', '-c=X'.
        if (a == "--config" || a == "-c") {
            if (i + 1 < args.size()) {
                r.config_path = args[i + 1]; i += 2; continue;
            }
            r.action = CliResult::Action::Error;
            r.error  = a + " needs a path argument";
            return r;
        }
        if (a.rfind("--config=", 0) == 0) {
            r.config_path = a.substr(9); ++i; continue;
        }
        if (a.rfind("-c=", 0) == 0) {
            r.config_path = a.substr(3); ++i; continue;
        }
        if (a == "--pomodoro") {
            r.action = CliResult::Action::RunDaemon;
            r.autostart_pomodoro = true;
            ++i; continue;
        }
        if (a == "run")     { r.action = CliResult::Action::RunDaemon; ++i; continue; }
        if (a == "toggle")  { r.action = CliResult::Action::ForwardToDaemon; r.request = "pet:toggle"; ++i; continue; }
        if (a == "quit")    { r.action = CliResult::Action::ForwardToDaemon; r.request = "pet:quit";   ++i; continue; }
        if (a == "status")  { r.action = CliResult::Action::ForwardToDaemon; r.request = "pet:status"; ++i; continue; }
        if (a == "waybar-status") {
            r.action = CliResult::Action::PrintWaybarStatus; ++i; continue;
        }
        if (a == "pomodoro") {
            // Two forms:
            //   aymm pomodoro              (no subcommand) -> RunDaemon + autostart
            //   aymm pomodoro <subcommand> -> ForwardToDaemon (existing)
            if (i + 1 < args.size()) {
                const auto& sub = args[i + 1];
                if (!sub.empty() && sub[0] != '-') {
                    r.action  = CliResult::Action::ForwardToDaemon;
                    r.request = "pomodoro:" + sub;
                    i += 2;
                    continue;
                }
            }
            r.action = CliResult::Action::RunDaemon;
            r.autostart_pomodoro = true;
            ++i; continue;
        }
        r.action = CliResult::Action::Error;
        r.error  = "unknown argument: " + a;
        return r;
    }
    return r;
}

} // namespace aymm
