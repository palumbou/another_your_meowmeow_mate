#include "hyprneko/ControlSocket.hpp"

#include <array>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

namespace hyprneko {

namespace {
std::filesystem::path runtime_dir() {
    if (const char* xdg = std::getenv("XDG_RUNTIME_DIR"); xdg && *xdg)
        return xdg;
    return "/tmp";
}
} // namespace

std::filesystem::path ControlSocket::default_path() {
    return runtime_dir() / "hyprneko" / "control.sock";
}

ControlSocket::~ControlSocket() { close_server(); }

void ControlSocket::close_server() {
    if (listen_fd_ >= 0) ::close(listen_fd_);
    listen_fd_ = -1;
    if (!bound_path_.empty()) {
        std::error_code ec;
        std::filesystem::remove(bound_path_, ec);
        bound_path_.clear();
    }
}

int ControlSocket::bind_listen() {
    const auto path = default_path();
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    // Stale socket cleanup.
    std::filesystem::remove(path, ec);

    listen_fd_ = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (listen_fd_ < 0) return -1;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    const auto s = path.string();
    if (s.size() >= sizeof(addr.sun_path)) { close_server(); return -1; }
    std::memcpy(addr.sun_path, s.c_str(), s.size() + 1);

    if (::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close_server();
        return -1;
    }
    if (::listen(listen_fd_, 4) < 0) { close_server(); return -1; }

    bound_path_ = path;
    ::chmod(s.c_str(), 0600);
    return listen_fd_;
}

bool ControlSocket::accept_one(const Handler& handler) {
    const int fd = ::accept4(listen_fd_, nullptr, nullptr, SOCK_CLOEXEC);
    if (fd < 0) return false;

    std::array<char, 256> buf{};
    const ssize_t n = ::recv(fd, buf.data(), buf.size() - 1, 0);
    std::string req = (n > 0) ? std::string(buf.data(), n) : std::string{};
    while (!req.empty() && (req.back() == '\n' || req.back() == '\r')) req.pop_back();

    std::string reply = handler ? handler(req) : "ok";
    if (reply.empty() || reply.back() != '\n') reply.push_back('\n');
    ::send(fd, reply.data(), reply.size(), MSG_NOSIGNAL);
    ::close(fd);
    return true;
}

std::string ControlSocket::send_request(const std::string& req, std::string* err) {
    const auto path = default_path();
    const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) { if (err) *err = "socket: " + std::string(::strerror(errno)); return {}; }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    const auto s = path.string();
    if (s.size() >= sizeof(addr.sun_path)) {
        if (err) *err = "socket path too long";
        ::close(fd); return {};
    }
    std::memcpy(addr.sun_path, s.c_str(), s.size() + 1);

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        if (err) *err = "no daemon (" + std::string(::strerror(errno)) + ")";
        ::close(fd);
        return {};
    }

    std::string line = req;
    if (line.empty() || line.back() != '\n') line.push_back('\n');
    ::send(fd, line.data(), line.size(), MSG_NOSIGNAL);

    std::array<char, 1024> buf{};
    const ssize_t n = ::recv(fd, buf.data(), buf.size() - 1, 0);
    ::close(fd);
    if (n <= 0) return {};
    std::string reply(buf.data(), n);
    while (!reply.empty() && (reply.back() == '\n' || reply.back() == '\r'))
        reply.pop_back();
    return reply;
}

} // namespace hyprneko
