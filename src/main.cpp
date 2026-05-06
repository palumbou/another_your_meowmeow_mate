#include "aymm/App.hpp"
#include "aymm/Cli.hpp"
#include "aymm/Config.hpp"
#include "aymm/ControlSocket.hpp"
#include "aymm/PomodoroTimer.hpp"
#include "aymm/WaybarStatus.hpp"

#include <iostream>

int main(int argc, char** argv) {
    using namespace aymm;

    auto cli = parse_cli(argc, argv);
    if (cli.action == CliResult::Action::PrintHelp)    { print_help(std::cout);    return 0; }
    if (cli.action == CliResult::Action::PrintVersion) { print_version(std::cout); return 0; }
    if (cli.action == CliResult::Action::Error) {
        std::cerr << "aymm: " << cli.error << "\n";
        print_help(std::cerr);
        return 2;
    }

    if (cli.action == CliResult::Action::ForwardToDaemon) {
        std::string err;
        const auto reply = ControlSocket::send_request(cli.request, &err);
        if (reply.empty() && !err.empty()) {
            std::cerr << "aymm: " << err << "\n";
            return 1;
        }
        std::cout << reply << "\n";
        return 0;
    }

    if (cli.action == CliResult::Action::PrintWaybarStatus) {
        // Prefer asking the running daemon so the timer state is authoritative;
        // if no daemon is running, emit a "stopped" payload so Waybar still
        // gets valid JSON.
        std::string err;
        const auto reply = ControlSocket::send_request("waybar-status", &err);
        if (!reply.empty()) {
            std::cout << reply << "\n";
            return 0;
        }
        PomodoroTimer dummy({});
        std::cout << waybar_json(dummy, /*pet_running=*/false) << "\n";
        return 0;
    }

    const auto path = cli.config_path.empty() ? Config::default_path()
                                               : std::filesystem::path(cli.config_path);
    auto loaded = Config::load(path);
    if (!loaded) {
        std::cerr << "aymm: no config at " << path
                  << " — using built-in defaults. Copy `examples/aymm.conf` from\n"
                     "      the source / install tree to that path to customize.\n";
    }
    Config cfg = loaded.value_or(Config{});

    // Apply CLI overrides on top of the loaded config. Negative / empty
    // means "leave the config value alone".
    if (cli.study_min                  > 0) cfg.pomodoro_study_minutes              = cli.study_min;
    if (cli.break_min                  > 0) cfg.pomodoro_break_minutes              = cli.break_min;
    if (cli.long_break_min             > 0) cfg.pomodoro_long_break_minutes         = cli.long_break_min;
    if (cli.sessions_before_long_break > 0) cfg.pomodoro_sessions_before_long_break = cli.sessions_before_long_break;
    if (!cli.focus_corner.empty()) {
        if      (cli.focus_corner == "bottom-right") cfg.pomodoro_focus_corner = FocusCorner::BottomRight;
        else if (cli.focus_corner == "bottom-left")  cfg.pomodoro_focus_corner = FocusCorner::BottomLeft;
        else if (cli.focus_corner == "top-right")    cfg.pomodoro_focus_corner = FocusCorner::TopRight;
        else if (cli.focus_corner == "top-left")     cfg.pomodoro_focus_corner = FocusCorner::TopLeft;
        else if (cli.focus_corner == "center")       cfg.pomodoro_focus_corner = FocusCorner::Center;
        else if (cli.focus_corner == "none")         cfg.pomodoro_focus_corner = FocusCorner::None;
        else {
            std::cerr << "aymm: invalid --focus-corner '" << cli.focus_corner
                      << "' (expected: bottom-right|bottom-left|top-right|top-left|center|none)\n";
            return 2;
        }
    }

    AppOptions opts;
    opts.autostart_pomodoro = cli.autostart_pomodoro;

    App app(std::move(cfg), opts);
    return app.run();
}
