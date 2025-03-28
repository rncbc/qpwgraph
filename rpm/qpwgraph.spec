#
# spec file for package qpwgraph
#
# Copyright (C) 2021-2025, rncbc aka Rui Nuno Capela. All rights reserved.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

Summary:	A PipeWire Graph Qt GUI Interface
Name:		qpwgraph
Version:	0.8.3
Release:	48.1
License:	GPL-2.0-or-later
Group:		Productivity/Multimedia/Sound/Midi
Source:		%{name}-%{version}.tar.gz
URL:		https://gitlab.freedesktop.org/rncbc/qpwgraph
#Packager:	rncbc.org

%if 0%{?fedora_version} >= 34 || 0%{?suse_version} > 1500 || ( 0%{?sle_version} == 150200 && 0%{?is_opensuse} )
%define qt_major_version  6
%else
%define qt_major_version  5
%endif

BuildRequires:	coreutils
BuildRequires:	pkgconfig
BuildRequires:	glibc-devel
BuildRequires:	gcc-c++
BuildRequires:	cmake >= 3.15
%if 0%{?sle_version} >= 150200 && 0%{?is_opensuse}
BuildRequires:	gcc10 >= 10
BuildRequires:	gcc10-c++ >= 10
%define _GCC	/usr/bin/gcc-10
%define _GXX	/usr/bin/g++-10
%else
BuildRequires:	gcc >= 10
BuildRequires:	gcc-c++ >= 10
%define _GCC	/usr/bin/gcc
%define _GXX	/usr/bin/g++
%endif
%if 0%{qt_major_version} == 6
%if 0%{?sle_version} == 150200 && 0%{?is_opensuse}
BuildRequires:	qtbase6.8-static >= 6.8
BuildRequires:	qttools6.8-static
BuildRequires:	qttranslations6.8-static
BuildRequires:	qtsvg6.8-static
%else
BuildRequires:	cmake(Qt6LinguistTools)
BuildRequires:	pkgconfig(Qt6Core)
BuildRequires:	pkgconfig(Qt6Gui)
BuildRequires:	pkgconfig(Qt6Widgets)
BuildRequires:	pkgconfig(Qt6Svg)
BuildRequires:	pkgconfig(Qt6Xml)
BuildRequires:	pkgconfig(Qt6Network)
%endif
%else
BuildRequires:	cmake(Qt5LinguistTools)
BuildRequires:	pkgconfig(Qt5Core)
BuildRequires:	pkgconfig(Qt5Gui)
BuildRequires:	pkgconfig(Qt5Widgets)
BuildRequires:	pkgconfig(Qt5Svg)
BuildRequires:	pkgconfig(Qt5Xml)
BuildRequires:	pkgconfig(Qt5Network)
%endif
BuildRequires:	pkgconfig(alsa)
BuildRequires:	pkgconfig(libpipewire-0.3)

%description
qpwgraph is a graph manager dedicated for PipeWire (https://pipewire.org),
using the Qt C++ framework (https://qt.io), based and pretty much like the
same of QjackCtl (https://qjackctl.sourceforge.io).


%prep
%setup -q

%build
%if 0%{?sle_version} == 150200 && 0%{?is_opensuse}
source /opt/qt6.8-static/bin/qt6.8-static-env.sh
%endif
CXX=%{_GXX} CC=%{_GCC} \
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCONFIG_ALSA_MIDI=ON -Wno-dev -B build
cmake --build build %{?_smp_mflags}

%install
DESTDIR="%{buildroot}" \
cmake --install build


%files
%license LICENSE.md
%doc README.md  ChangeLog
%dir %{_datadir}/applications
%dir %{_datadir}/icons/hicolor
%dir %{_datadir}/icons/hicolor/32x32
%dir %{_datadir}/icons/hicolor/32x32/apps
%dir %{_datadir}/icons/hicolor/32x32/mimetypes
%dir %{_datadir}/icons/hicolor/scalable
%dir %{_datadir}/icons/hicolor/scalable/apps
%dir %{_datadir}/icons/hicolor/scalable/mimetypes
%dir %{_datadir}/metainfo
%dir %{_datadir}/man
%dir %{_datadir}/man/man1
%{_bindir}/%{name}
%{_datadir}/applications/org.rncbc.%{name}.desktop
%{_datadir}/icons/hicolor/32x32/apps/org.rncbc.%{name}.png
%{_datadir}/icons/hicolor/scalable/apps/org.rncbc.%{name}.svg
%{_datadir}/metainfo/org.rncbc.%{name}.metainfo.xml
%{_datadir}/mime/packages/org.rncbc.%{name}.xml
%{_datadir}/icons/hicolor/32x32/mimetypes/org.rncbc.%{name}.application-x-%{name}*.png
%{_datadir}/icons/hicolor/scalable/mimetypes/org.rncbc.%{name}.application-x-%{name}*.svg
%{_datadir}/man/man1/%{name}.1.gz


%changelog
* Fri Mar 28 2025 Rui Nuno Capela <rncbc@rncbc.org> 0.8.3
- An early-spring'25 beta release.
* Fri Mar  7 2025 Rui Nuno Capela <rncbc@rncbc.org> 0.8.2
- An end-of-winter'25 beta release.
* Fri Dec 27 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.8.1
- An end-of-year'24 beta release.
* Thu Nov 14 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.8.0
- A mid-autumn'24 beta release.
