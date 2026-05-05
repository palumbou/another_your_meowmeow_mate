#include "aymm/HyprlandCursorProvider.hpp"

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace aymm {

namespace {

bool path_exists(const std::string& p) {
    std::error_code ec;
    return std::filesystem::exists(p, ec);
}

} // namespace

std::string HyprlandCursorProvider::detect_socket_path() {
    const char* sig = std::getenv("HYPRLAND_INSTANCE_SIGNATURE");
    if (!sig || !*sig) return {};

    if (const char* xdg = std::getenv("XDG_RUNTIME_DIR"); xdg && *xdg) {
        std::string p = std::string(xdg) + "/hypr/" + sig + "/.socket.sock";
        if (path_exists(p)) return p;
    }
    // Legacy fallback for older Hyprland versions.
    std::string legacy = std::string("/tmp/hypr/") + sig + "/.socket.sock";
    if (path_exists(legacy)) return legacy;
    return {};
}

HyprlandCursorProvider::HyprlandCursorProvider()
    : socket_path_(detect_socket_path()) {}

std::optional<CursorPos> HyprlandCursorProvider::poll() {
    if (socket_path_.empty()) return std::nullopt;

    const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) return std::nullopt;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    if (socket_path_.size() >= sizeof(addr.sun_path)) { ::close(fd); return std::nullopt; }
    std::memcpy(addr.sun_path, socket_path_.c_str(), socket_path_.size() + 1);

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return std::nullopt;
    }

    static constexpr char kCmd[] = "cursorpos";
    if (::send(fd, kCmd, sizeof(kCmd) - 1, MSG_NOSIGNAL) < 0) {
        ::close(fd);
        return std::nullopt;
    }

    std::array<char, 64> buf{};
    const ssize_t n = ::recv(fd, buf.data(), buf.size() - 1, 0);
    ::close(fd);
    if (n <= 0) return std::nullopt;

    // Hyprland replies "X, Y\n" (or "X,Y") in compositor global coordinates.
    int x = 0, y = 0;
    if (std::sscanf(buf.data(), "%d, %d", &x, &y) != 2 &&
        std::sscanf(buf.data(), "%d,%d",  &x, &y) != 2) {
        return std::nullopt;
    }
    return CursorPos{x, y};
}

} // namespace aymm
