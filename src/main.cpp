#include "hyprneko/App.hpp"
#include "hyprneko/Cli.hpp"
#include "hyprneko/Config.hpp"
#include "hyprneko/ControlSocket.hpp"
#include "hyprneko/PomodoroTimer.hpp"
#include "hyprneko/WaybarStatus.hpp"

#include <iostream>

int main(int argc, char** argv) {
    using namespace hyprneko;

    auto cli = parse_cli(argc, argv);
    if (cli.action == CliResult::Action::PrintHelp)    { print_help(std::cout);    return 0; }
    if (cli.action == CliResult::Action::PrintVersion) { print_version(std::cout); return 0; }
    if (cli.action == CliResult::Action::Error) {
        std::cerr << "hyprneko: " << cli.error << "\n";
        print_help(std::cerr);
        return 2;
    }

    if (cli.action == CliResult::Action::ForwardToDaemon) {
        std::string err;
        const auto reply = ControlSocket::send_request(cli.request, &err);
        if (reply.empty() && !err.empty()) {
            std::cerr << "hyprneko: " << err << "\n";
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
    Config cfg = Config::load_or_default(path);

    App app(std::move(cfg));
    return app.run();
}
