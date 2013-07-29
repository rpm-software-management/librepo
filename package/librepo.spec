%global gitrev 024ef3d
# gitrev is output of: git rev-parse --short HEAD

Name:		librepo
Version:	0.0.5
Release:	3%{?dist}
Summary:	Repodata downloading library

Group:		System Environment/Libraries
License:	GPLv2+
URL:		https://github.com/Tojaj/librepo
# Use the following commands to generate the tarball:
#  git clone https://github.com/Tojaj/librepo.git
#  cd librepo
#  utils/make_tarball.sh %{gitrev}
Source0:	librepo-%{gitrev}.tar.xz

BuildRequires:	check-devel
BuildRequires:	cmake
BuildRequires:	doxygen
BuildRequires:	expat-devel
BuildRequires:  glib2-devel >= 2.22.0
BuildRequires:	gpgme-devel
BuildRequires:	libattr-devel
BuildRequires:	libcurl-devel
BuildRequires:	openssl-devel
BuildRequires:	pygpgme
BuildRequires:	python2-devel
BuildRequires:	python-flask
BuildRequires:	python-nose
BuildRequires:	python-sphinx

%description
A library providing C and Python (libcURL like) API to downloading repository
metadata.

%package devel
Summary:	Repodata downloading library
Group:		Development/Libraries
Requires:	%{name}%{?_isa} = %{version}-%{release}

%description devel
Development files for librepo.

%package -n python-librepo
Summary:	Python bindings for the librepo library
Group:		Development/Languages
Requires:	%{name}%{?_isa} = %{version}-%{release}

%description -n python-librepo
Python bindings for the librepo library.

%prep
%setup -q -n librepo

%build
%cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .
make %{?_smp_mflags}

%check
make ARGS="-V" test

%install
make install DESTDIR=$RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%doc COPYING README.md
%{_libdir}/librepo.so.*

%files devel
%{_libdir}/librepo.so
%{_libdir}/pkgconfig/librepo.pc
%{_includedir}/librepo/

%files -n python-librepo
%{python_sitearch}/librepo/

%changelog
* Thu Jul  25 2013 Tomas Mlcoch <tmlcoch at redhat.com> - 0.0.5-3
- python: Raise exception if handle has bad repo type configured
  (RhBug: 988013)

* Mon Jul  22 2013 Tomas Mlcoch <tmlcoch at redhat.com> - 0.0.5-2
- Bump version in versioh.h to 0.0.5
- Python: Fix Handle.mirrors to return empty list instead of None if
  no mirrors available (RhBug: 986228)

* Wed Jul  17 2013 Tomas Mlcoch <tmlcoch at redhat.com> - 0.0.5-1
- Return LRE_ALREADYDOWNLOADED if the file exists even if no resume
  is specified. (GitHub issue 15)
- downloadtarget: New module, future replacement for curltarget module.
- Librepo migrated to lr_LrMirrorlist from lr_InternalMirrorlist.
- test: Run python unittest verbosely
- lrmirrorlis: New module. GLib2 ready replacement for the internal_mirrorlist
  module.
- package_downloader: Add LRE_ALREADYDOWNLOADED rc code. (GitHub issue 15)
- handle: After set python SIGINT handler back, check if librepo was
  interrupted by CTRL+C. (RhBug: 977803)
- cmake: Set required python version to 2. (GitHub issue 10)
- Fix missing VAR substitution for mirrorlist. (GitHub issue 11)
- cmake: Add FindXattr module.
- Add support for caching checksum as extended file attribute. (GitHub issue 8)
- util: Add lr_asprintf().
- util: Add lr_vasprintf().
- handle: Fix funky logic in internal error handling. (GitHub issue 9)
- Add lr_yum_repomd_get_age() function. (GitHub issue 6)
- test: Add test for LR_VERSION_CHECK macro.
- Add a LR_VERSION_CHECK macro

* Wed Jun 12 2013 Tomas Mlcoch <tmlcoch at redhat.com> - 0.0.4-2
- examples: Add C example of usage.
- Fix predefined lists in types.h (GitHub issue 4). Thank you hughsie
- Add LRO_VARSUB and LRI_VARSUB. (RhBug: 965131)
- py: Change reported name from _librepo.Exception to librepo.LibrepoException

* Thu May  2 2013 Tomas Mlcoch <tmlcoch at redhat.com> - 0.0.4-1
- Fix type conversion long long -> long.(RhBug: 957656)
- python: Handle.perfrom() could be called without Result().
- Add LRI_MAXMIRRORTRIES option. (RhBug: 954736)
- py: unittests: Add metalink.xml and mirrorlist files. (RhBug: 954294)
- Fix double free and memory leak. (RhBug: 954294)
- New option LRO_MAXMIRRORTRIES. (RhBug: 949517)
- LRI_MIRRORS return only content of mirrorlist file (without LRO_URL as first item).
- Add LRO_FETCHMIRRORS option.

* Mon Apr  8 2013 Tomas Mlcoch <tmlcoch at redhat.com> - 0.0.2-3.git720d68d
- Add CURL_GLOBAL_ACK_EINTR flag to curl init.
- Proper multi handle cleanup. (RhBug: 947388)
- Support for read 'useragent' attr. (RhBug: 947346)
- Add valgrind supress files. (RhBug: 923214)
- Make python bindings interruptible (LRO_INTERRUPTIBLE) (RhBug: 919125)
- Add LRI_MIRRORS option (RhBug: 923198)
- Add LRI_METALINK option. (BzBug: 947767)

* Mon Mar 18 2013 Tomas Mlcoch <tmlcoch at redhat.com> - 0.0.2-2.gitb3c3323
- py: Use standard python exception while accessing bad attrs. (RhBug: 920673)
- Default mask for newly created files is 0666. (RhBug: 922557)

* Thu Mar 14 2013 Tomas Mlcoch <tmlcoch at redhat.com> - 0.0.2-1.git714e828
- Add LRI_PROGRESSCB and LRI_PROGRESSDATA options (RhBug: 919123)
- Bindings: More pythonic operations with handle's attributes (RhBug: 919124)

* Tue Oct  9 2012 Tomas Mlcoch <tmlcoch at redhat.com> - 0.0.1-1.gitc69642e
- Initial package
