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
Version:	0.9.6
Release:	55.1
License:	GPL-2.0-or-later
Group:		Productivity/Multimedia/Sound/Midi
Source:		%{name}-%{version}.tar.gz
URL:		https://gitlab.freedesktop.org/rncbc/qpwgraph
#Packager:	rncbc.org

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
%if 0%{?sle_version} == 150200 && 0%{?is_opensuse}
BuildRequires:	qtbase6.9-static >= 6.9
BuildRequires:	qttools6.9-static
BuildRequires:	qttranslations6.9-static
BuildRequires:	qtsvg6.9-static
%else
BuildRequires:	cmake(Qt6LinguistTools)
BuildRequires:	pkgconfig(Qt6Core)
BuildRequires:	pkgconfig(Qt6Gui)
BuildRequires:	pkgconfig(Qt6Widgets)
BuildRequires:	pkgconfig(Qt6Svg)
BuildRequires:	pkgconfig(Qt6Xml)
BuildRequires:	pkgconfig(Qt6Network)
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
source /opt/qt6.9-static/bin/qt6.9-static-env.sh
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
* Mon Oct  6 2025 Rui Nuno Capela <rncbc@rncbc.org> 0.9.6
- An early-fall'25 beta release.
* Fri Aug 15 2025 Rui Nuno Capela <rncbc@rncbc.org> 0.9.5
- A mid-summer'25 beta release.
* Sat Jun 21 2025 Rui Nuno Capela <rncbc@rncbc.org> 0.9.4
- An early-summer'25 beta release.
* Mon Jun  2 2025 Rui Nuno Capela <rncbc@rncbc.org> 0.9.3
- An end-of-spring'25 beta release.
* Sat May 17 2025 Rui Nuno Capela <rncbc@rncbc.org> 0.9.2
- A mid-spring'25 hot-fix beta release.
* Thu May 15 2025 Rui Nuno Capela <rncbc@rncbc.org> 0.9.1
- A mid-spring'25 beta release.
* Fri Apr 25 2025 Rui Nuno Capela <rncbc@rncbc.org> 0.9.0
- A spring'25 beta release.
