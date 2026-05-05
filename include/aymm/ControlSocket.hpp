#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace aymm {

// Tiny line-based protocol over an AF_UNIX socket. Each line is a request
// like `pomodoro:start` or `pet:toggle`; the daemon replies with a single
// line. Used by `aymm <subcommand>` to talk to the running pet daemon.
//
// Path: $XDG_RUNTIME_DIR/aymm/control.sock (falls back to /tmp).
class ControlSocket {
public:
    using Handler = std::function<std::string(const std::string& request)>;

    static std::filesystem::path default_path();

    // Server side. Binds, listens, and registers the listener with the
    // provided poll fd integration. Returns -1 fd on error.
    int  bind_listen();
    int  listen_fd() const { return listen_fd_; }
    bool accept_one(const Handler& handler);
    void close_server();

    // Client side. Connects, sends request, reads reply (single line).
    static std::string send_request(const std::string& req,
                                    std::string* err = nullptr);

    ~ControlSocket();

private:
    int listen_fd_ = -1;
    std::filesystem::path bound_path_;
};

} // namespace aymm
