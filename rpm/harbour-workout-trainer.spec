# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 Michele Brami

Name:       harbour-workout-trainer
Summary:    Workout Trainer
Version:    0.1.0
Release:    1
License:    GPLv3+
URL:        https://github.com/mbrami76/harbour-workout-trainer
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5 >= 0.10.9
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Sql)
BuildRequires:  pkgconfig(Qt5Multimedia)
BuildRequires:  qt5-qttools-linguist
BuildRequires:  desktop-file-utils

# The speech libraries travel with the app and are found through the rpath of the binary.
%define __provides_exclude_from ^%{_datadir}/%{name}/lib/.*$
%define __requires_exclude ^(libonnxruntime|libsherpa-onnx-c-api)\\.so.*$

%description
Workout timer for repetition, interval and circuit training.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
%make_build

%install
%qmake5_install
%{__strip} --strip-unneeded %{buildroot}%{_bindir}/%{name}
desktop-file-install --delete-original --dir %{buildroot}%{_datadir}/applications %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png

%changelog
* Wed Nov 04 2026 Michele Brami <info@michelebrami.it> - 0.1.0-1
- Project skeleton
