Name:           hyprneko
Version:        0.1.0
Release:        1%{?dist}
Summary:        Wayland-native desktop pet that chases your cursor under Hyprland

License:        MIT
URL:            https://example.invalid/hyprneko
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
Hyprneko is a Wayland-native desktop pet inspired by oneko. It runs as an
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
%{_bindir}/hyprneko
%{_datadir}/hyprneko/
%{_datadir}/applications/hyprneko.desktop
%{_libdir}/systemd/user/hyprneko.service

%changelog
* Mon May 04 2026 Ugo Palumbo <ugo.palumbo@example> - 0.1.0-1
- Initial package.
