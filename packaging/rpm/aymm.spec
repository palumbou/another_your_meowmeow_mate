Name:           aymm
Version:        0.1.0
Release:        1%{?dist}
Summary:        Wayland-native desktop pet that chases your cursor under Hyprland

License:        MIT
URL:            https://example.invalid/aymm
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  wayland-devel
BuildRequires:  wayland-protocols-devel
BuildRequires:  pkgconfig(cairo)

Requires:       cairo
Requires:       wayland

%description
Another Your MeowMeow Mate is a Wayland-native desktop pet inspired by oneko. It runs as an
overlay layer-shell surface and chases the global cursor by polling Hyprland's
IPC socket — no X11/XWayland involved. Includes an optional Pomodoro mode
and a Waybar custom-module integration.

%prep
%autosetup -p1

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%files
%license LICENSE
%doc README.md
%{_bindir}/aymm
%{_datadir}/aymm/
%{_datadir}/applications/aymm.desktop
%{_libdir}/systemd/user/aymm.service

%changelog
* Mon May 04 2026 Ugo Palumbo <ugo.palumbo@example> - 0.1.0-1
- Initial package.
