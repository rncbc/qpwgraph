#
# spec file for package qpwgraph
#
# Copyright (C) 2021-2024, rncbc aka Rui Nuno Capela. All rights reserved.
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
Version:	0.8.0
Release:	45.1
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
* Thu Nov 14 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.8.0
- A mid-autumn'24 beta release.
* Mon Oct 28 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.7.9
- An autumn'24 beta release.
* Thu Sep 19 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.7.8
- An end-of-summer'24 beta release.
* Wed Aug 21 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.7.7
- A mid-summer'24 beta release.
* Fri Jul 12 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.7.5
- A summer'24 beta release.
* Fri Jun 28 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.7.4
- An early-summer'24 hot-fix release.
* Sat Jun 22 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.7.3
- An early-summer'24 beta release.
* Sun May 12 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.7.2
- A mid-spring'24 beta release.
* Thu Apr 25 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.7.1
- A spring'24 beta release hot-fix.
* Mon Apr 22 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.7.0
- A spring'24 beta release.
* Fri Mar 29 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.6.3
- A good-friday'24 release.
* Mon Jan 22 2024 Rui Nuno Capela <rncbc@rncbc.org> 0.6.2
- A winter'24 release.
* Sat Dec  2 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.6.1
- An end-of-autumn'23 release.
* Wed Nov  8 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.6.0
- An autumn'23 release.
* Fri Sep  8 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.5.3
- An end-of-summer'23 release.
* Sat Aug  5 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.5.2
- A high-summer'23 release.
* Mon Jul 17 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.5.1
- A summer'23 hot-fix release.
* Sun Jul 16 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.5.0
- Yet another summer'23 release.
* Mon Jul 10 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.4.5
- A summer'23 release.
* Sun Jun 18 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.4.4
- A late-spring'23 regression.
* Sat Jun 17 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.4.3
- A late-spring'23 release.
* Sun Apr  2 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.4.2
- An early-spring'23 release.
* Fri Mar  3 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.4.1
- A late-winter'23 release.
* Sat Feb 25 2023 Rui Nuno Capela <rncbc@rncbc.org> 0.4.0
- A mid-winter'23 release.
* Tue Dec 27 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.3.9
- An end-of-year'22 release.
* Sat Nov 19 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.3.8
- A mid-autumn'22 release.
* Sat Oct 22 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.3.7
- An autumn'22 release.
* Sat Sep 24 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.3.6
- An early-autumn'22 release.
* Sat Aug 20 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.3.5
- A thirteenth beta release.
* Fri Jul  8 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.3.4
- A twelfth beta release.
* Wed Jul  6 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.3.3
- An eleventh beta release.
* Mon Jun 13 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.3.2
- A tenth beta release.
* Sun May 29 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.3.1
- A ninth beta release.
* Sat May 21 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.3.0
- An eighth beta release.
* Sat Apr 23 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.2.6
- A seventh beta release.
* Wed Apr  6 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.2.5
- A sixth beta release.
* Sat Mar 19 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.2.4
- A fifth beta release.
* Sat Mar 12 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.2.3
- A fourth beta release.
* Wed Mar  2 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.2.2
- A thrice beta than before.
* Sat Feb 26 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.2.1
- Just a second beta.
- Patchbay feature introduced.
* Sun Jan 16 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.2.0
- Enter first beta.
* Thu Jan 13 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.1.3
- A Winter'22 Release.
* Sat Jan  1 2022 Rui Nuno Capela <rncbc@rncbc.org> 0.1.2
- One third alpha.
* Sat Dec 18 2021 Rui Nuno Capela <rncbc@rncbc.org> 0.1.1
- One second alpha.
* Mon Dec  6 2021 Rui Nuno Capela <rncbc@rncbc.org> 0.1.0
- One first alpha.
