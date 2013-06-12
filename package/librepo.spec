%global gitrev ffbc1d6
# gitrev is output of: git rev-parse --short HEAD

Name:		librepo
Version:	0.0.4
Release:	2%{?dist}
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
BuildRequires:	gpgme-devel
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
* Wed Jun 12 2013 Tomas Mlcoch <tmlcoch at redhat.com> - 0.0.4-2
- Fix predefined lists in types.h (GitHub issue 4). Thank you hughsie

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

* Thu Oct  9 2012 Tomas Mlcoch <tmlcoch at redhat.com> - 0.0.1-1.gitc69642e
- Initial package
