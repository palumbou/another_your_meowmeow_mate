#include "aymm/EvdevCursorProvider.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <linux/input.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

namespace aymm {

namespace {

constexpr unsigned long bit_word(unsigned long n) { return n / (8 * sizeof(unsigned long)); }
constexpr unsigned long bit_mask(unsigned long n) { return 1UL << (n % (8 * sizeof(unsigned long))); }

bool device_has_rel_xy(int fd) {
    unsigned long evbit[(EV_MAX  + 8 * sizeof(unsigned long)) / (8 * sizeof(unsigned long))] = {0};
    unsigned long relbit[(REL_MAX + 8 * sizeof(unsigned long)) / (8 * sizeof(unsigned long))] = {0};
    if (::ioctl(fd, EVIOCGBIT(0,     sizeof(evbit)),  evbit)  < 0) return false;
    if (!(evbit[bit_word(EV_REL)]   & bit_mask(EV_REL)))            return false;
    if (::ioctl(fd, EVIOCGBIT(EV_REL, sizeof(relbit)), relbit) < 0) return false;
    const bool has_x = relbit[bit_word(REL_X)] & bit_mask(REL_X);
    const bool has_y = relbit[bit_word(REL_Y)] & bit_mask(REL_Y);
    return has_x && has_y;
}

} // namespace

EvdevCursorProvider::EvdevCursorProvider() {
    DIR* dir = ::opendir("/dev/input");
    if (!dir) return;

    bool any_perm_denied = false;
    while (auto* ent = ::readdir(dir)) {
        if (std::strncmp(ent->d_name, "event", 5) != 0) continue;
        const std::string path = std::string("/dev/input/") + ent->d_name;
        const int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) {
            if (errno == EACCES || errno == EPERM) any_perm_denied = true;
            continue;
        }
        if (device_has_rel_xy(fd)) {
            fds_.push_back(fd);
        } else {
            ::close(fd);
        }
    }
    ::closedir(dir);

    if (fds_.empty()) {
        std::cerr << "aymm: evdev provider could not open any pointer device";
        if (any_perm_denied) {
            std::cerr << " (permission denied — add your user to the `input` group: "
                         "`sudo gpasswd -a $USER input` then re-login)";
        }
        std::cerr << ".\n";
    }
}

EvdevCursorProvider::~EvdevCursorProvider() {
    for (int fd : fds_) ::close(fd);
}

void EvdevCursorProvider::set_bounds(int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    bx_ = x; by_ = y; bw_ = w; bh_ = h;
    cx_ = std::clamp(cx_, double(bx_), double(bx_ + bw_ - 1));
    cy_ = std::clamp(cy_, double(by_), double(by_ + bh_ - 1));
}

void EvdevCursorProvider::set_position(int x, int y) {
    cx_ = std::clamp(double(x), double(bx_), double(bx_ + bw_ - 1));
    cy_ = std::clamp(double(y), double(by_), double(by_ + bh_ - 1));
}

std::optional<CursorPos> EvdevCursorProvider::poll() {
    if (fds_.empty()) return std::nullopt;

    input_event ev{};
    for (int fd : fds_) {
        for (;;) {
            const ssize_t n = ::read(fd, &ev, sizeof(ev));
            if (n != static_cast<ssize_t>(sizeof(ev))) break;
            if (ev.type != EV_REL) continue;
            if (ev.code == REL_X) cx_ += ev.value;
            else if (ev.code == REL_Y) cy_ += ev.value;
        }
    }
    cx_ = std::clamp(cx_, double(bx_), double(bx_ + bw_ - 1));
    cy_ = std::clamp(cy_, double(by_), double(by_ + bh_ - 1));
    return CursorPos{ static_cast<int>(cx_), static_cast<int>(cy_) };
}

} // namespace aymm
