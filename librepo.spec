%if 0%{?rhel} && 0%{?rhel} <= 7
# Do not build bindings for python3 for RHEL <= 7
%bcond_with python3
# python-flask is not in RHEL7
%bcond_with tests
%else
%bcond_without python3
%bcond_without tests
%endif
%global dnf_conflict 2.8.8

Name:           librepo
Version:        1.9.0
Release:        1%{?dist}
Summary:        Repodata downloading library

License:        LGPLv2+
URL:            https://github.com/rpm-software-management/librepo
Source0:        %{url}/archive/%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc
BuildRequires:  check-devel
BuildRequires:  doxygen
BuildRequires:  glib2-devel >= 2.26.0
BuildRequires:  gpgme-devel
BuildRequires:  libattr-devel
BuildRequires:  libcurl-devel >= 7.19.0
BuildRequires:  libxml2-devel
BuildRequires:  openssl-devel

%description
A library providing C and Python (libcURL like) API to downloading repository
metadata.

%package devel
Summary:        Repodata downloading library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
Development files for librepo.

%package -n python2-%{name}
Summary:        Python bindings for the librepo library
%{?python_provide:%python_provide python2-%{name}}
%if 0%{?rhel} && 0%{?rhel} <= 7
BuildRequires:  python-sphinx
BuildRequires:  pygpgme
%else
BuildRequires:  python2-sphinx
BuildRequires:  python2-pygpgme
%endif
BuildRequires:  python2-devel
%if (0%{?rhel} && 0%{?rhel} <= 7) || (0%{?fedora} && 0%{?fedora} <= 27)
BuildRequires:  pyxattr
%else
BuildRequires:  python2-pyxattr
%endif
%if %{with tests}
BuildRequires:  python2-flask
BuildRequires:  python2-nose
%endif
Requires:       %{name}%{?_isa} = %{version}-%{release}
Conflicts:      python2-dnf < %{dnf_conflict}

%description -n python2-%{name}
Python 2 bindings for the librepo library.

%if %{with python3}
%package -n python3-%{name}
Summary:        Python 3 bindings for the librepo library
%{?system_python_abi}
%{?python_provide:%python_provide python3-%{name}}
BuildRequires:  python3-pygpgme
BuildRequires:  python3-devel
%if %{with tests}
BuildRequires:  python3-flask
BuildRequires:  python3-nose
%endif
BuildRequires:  python3-sphinx
BuildRequires:  python3-pyxattr
Requires:       %{name}%{?_isa} = %{version}-%{release}
Conflicts:      python3-dnf < %{dnf_conflict}

%description -n python3-%{name}
Python 3 bindings for the librepo library.
%endif

%prep
%autosetup -p1

mkdir build build-py3

%build
pushd build
  %cmake ..
  %make_build
popd

%if %{with python3}
pushd build-py3
  %cmake -DPYTHON_DESIRED:str=3 ..
  %make_build
popd
%endif

%if %{with tests}
%check
pushd build
  #ctest -VV
  make ARGS="-V" test
popd

%if %{with python3}
pushd build-py3
  #ctest -VV
  make ARGS="-V" test
popd
%endif
%endif

%install
pushd build
  %make_install
popd
%if %{with python3}
pushd build-py3
  %make_install
popd
%endif

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%license COPYING
%doc README.md
%{_libdir}/%{name}.so.*

%files devel
%{_libdir}/%{name}.so
%{_libdir}/pkgconfig/%{name}.pc
%{_includedir}/%{name}/

%files -n python2-%{name}
%{python2_sitearch}/%{name}/

%if %{with python3}
%files -n python3-%{name}
%{python3_sitearch}/%{name}/
%endif

%changelog
