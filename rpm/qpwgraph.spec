#
# spec file for package qmidinet
#
# Copyright (C) 2021-2022, rncbc aka Rui Nuno Capela. All rights reserved.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.
#
# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

%define name	qpwgraph
%define version	0.3.6
%define release	18.1

%define _prefix	/usr

%if %{defined fedora}
%define debug_package %{nil}
%endif

%if 0%{?fedora_version} >= 34 || 0%{?suse_version} > 1500
%define qt_major_version  6
%else
%define qt_major_version  5
%endif

Summary:	A PipeWire Graph Qt GUI Interface
Name:		%{name}
Version:	%{version}
Release:	%{release}
License:	GPL-2.0+
Group:		Productivity/Multimedia/Sound/Midi
Source0:	%{name}-%{version}.tar.gz
URL:		https://gitlab.freedesktop.org/rncbc/qpwgraph
Packager:	rncbc.org

BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-buildroot

BuildRequires:	coreutils
BuildRequires:	pkgconfig
BuildRequires:	glibc-devel
BuildRequires:	gcc-c++
%if %{defined fedora} || 0%{?suse_version} > 1500
BuildRequires:	gcc-c++ >= 8
%define CXX		/usr/bin/g++
%else
BuildRequires:	gcc8-c++ >= 8
%define CXX		/usr/bin/g++-8
%endif
%if %{defined fedora}
%if 0%{qt_major_version} == 6
BuildRequires:	qt6-qtbase-devel >= 6.1
BuildRequires:	qt6-qttools-devel
BuildRequires:	qt6-qtwayland-devel
BuildRequires:	qt6-qtsvg-devel
BuildRequires:	qt6-linguist
%else
BuildRequires:	qt5-qtbase-devel >= 5.1
BuildRequires:	qt5-qttools-devel
BuildRequires:	qt5-qtwayland-devel
BuildRequires:	qt5-qtsvg-devel
BuildRequires:	qt5-linguist
%endif
BuildRequires:	alsa-lib-devel
%else
%if 0%{qt_major_version} == 6
BuildRequires:	qt6-base-devel >= 6.1
BuildRequires:	qt6-tools-devel
BuildRequires:	qt6-wayland-devel
BuildRequires:	qt6-svg-devel
BuildRequires:	qt6-linguist-devel
%else
BuildRequires:	libqt5-qtbase-devel >= 5.1
BuildRequires:	libqt5-qttools-devel
BuildRequires:	libqt5-qtwayland-devel
BuildRequires:	libqt5-qtsvg-devel
BuildRequires:	libqt5-linguist-devel
%endif
BuildRequires:	alsa-devel
%endif
BuildRequires:	pipewire-devel

%if %{defined fedora}
BuildRequires:	liblilv-0-0 libsratom-0-0 libsord-0-0 libserd-0-0
%endif

%description
qpwgraph is a graph manager dedicated for PipeWire (https://pipewire.org),
using the Qt C++ framework (https://qt.io), based and pretty much like the
same of QjackCtl (https://qjackctl.sourceforge.io).

%prep
%setup -q

%build
CXX=%{CXX} \
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCONFIG_ALSA_MIDI=ON -Wno-dev -B build
cmake --build build %{?_smp_mflags}

%install
DESTDIR="%{buildroot}" \
cmake --install build

%clean
[ -d "%{buildroot}" -a "%{buildroot}" != "/" ] && %__rm -rf "%{buildroot}"

%files
%defattr(-,root,root)
%doc README.md LICENSE.md ChangeLog
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
