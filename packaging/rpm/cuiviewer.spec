Name:           cuiviewer
Version:        0.1.0
Release:        1%{?dist}
Summary:        Native image viewer (zgv-inspired)

License:        GPL-3.0-or-later
URL:            https://github.com/pdenapo/cuiviewer
Source0:        https://github.com/pdenapo/cuiviewer/releases/download/v%{version}/cuiviewer-%{version}.tar.gz

BuildRequires:  cmake, ninja-build, gcc, glfw-devel, mesa-libGL-devel, libX11-devel
Requires:       glfw, mesa-libGL, libX11

%description
Blazing-fast native image viewer built with CUI.
Includes the cuiviewer GUI application.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCUI_PLATFORM=glfw
%cmake_build

%install
%cmake_install
install -Dm644 README.md %{buildroot}%{_docdir}/%{name}/README.md

%files
%license LICENSE
%doc README.md
%{_bindir}/cuiviewer
