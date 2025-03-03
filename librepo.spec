%global libcurl_version 7.52.0

%undefine __cmake_in_source_build

%if 0%{?rhel}
%bcond_with zchunk
%else
%bcond_without zchunk
%endif

%if 0%{?fedora} >= 39 || 0%{?rhel} >= 10
%bcond_with use_gpgme
%bcond_with use_selinux
%else
%bcond_without use_gpgme
%bcond_without use_selinux
%endif

# Needs to match how gnupg2 is compiled
%bcond_with run_gnupg_user_socket

%bcond_with sanitizers

%if %{with use_gpgme} && %{with use_selinux}
%global need_selinux 1
%else
%global need_selinux 0
%endif

%global dnf_conflict 2.8.8

Name:           librepo
Version:        1.19.0
Release:        1%{?dist}
Summary:        Repodata downloading library

License:        LGPL-2.1-or-later
URL:            https://github.com/rpm-software-management/librepo
Source0:        %{url}/archive/%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc
BuildRequires:  check-devel
BuildRequires:  doxygen
BuildRequires:  pkgconfig(glib-2.0) >= 2.66
%if %{with use_gpgme}
BuildRequires:  gpgme-devel
%else
BuildRequires:  pkgconfig(rpm) >= 4.18.0
%endif
BuildRequires:  libattr-devel
BuildRequires:  libcurl-devel >= %{libcurl_version}
BuildRequires:  pkgconfig(json-glib-1.0)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(libcrypto)
%if %{need_selinux}
BuildRequires:  pkgconfig(libselinux)
%endif
BuildRequires:  pkgconfig(openssl)
%if %{with zchunk}
BuildRequires:  pkgconfig(zck) >= 0.9.11
%endif
Requires:       libcurl%{?_isa} >= %{libcurl_version}

%if %{with sanitizers}
BuildRequires:  libasan
BuildRequires:  liblsan
BuildRequires:  libubsan
%endif

%description
A library providing C and Python (libcURL like) API to downloading repository
metadata.

%package devel
Summary:        Repodata downloading library
Requires:       %{name}%{?_isa} = %{version}-%{release}
%if %{with zchunk}
Requires:       zchunk-devel%{?_isa}
%endif

%description devel
Development files for librepo.

%package -n python3-%{name}
Summary:        Python 3 bindings for the librepo library
%{?python_provide:%python_provide python3-%{name}}
BuildRequires:  python3-devel
BuildRequires:  python3-gpg
BuildRequires:  python3-pyxattr
BuildRequires:  python3-requests
BuildRequires:  python3-sphinx
Requires:       %{name}%{?_isa} = %{version}-%{release}
# Obsoletes Fedora 27 package
Obsoletes:      platform-python-%{name} < %{version}-%{release}
Conflicts:      python3-dnf < %{dnf_conflict}

%description -n python3-%{name}
Python 3 bindings for the librepo library.

%prep
%autosetup -p1

%build
%cmake \
    -DWITH_ZCHUNK=%{?with_zchunk:ON}%{!?with_zchunk:OFF} \
    -DUSE_GPGME=%{?with_use_gpgme:ON}%{!?with_use_gpgme:OFF} \
    -DUSE_RUN_GNUPG_USER_SOCKET=%{?with_run_gnupg_user_socket:ON}%{!?with_run_gnupg_user_socket:OFF} \
    -DWITH_SANITIZERS=%{?with_sanitizers:ON}%{!?with_sanitizers:OFF} \
%if %{need_selinux}
    -DENABLE_SELINUX=ON
%else
    -DENABLE_SELINUX=OFF
%endif
%cmake_build

%check
%ctest

%install
%cmake_install

%if 0%{?rhel} && 0%{?rhel} <= 7
%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
%else
%ldconfig_scriptlets
%endif

%files
%license COPYING
%doc README.md
%{_libdir}/%{name}.so.*

%files devel
%{_libdir}/%{name}.so
%{_libdir}/pkgconfig/%{name}.pc
%{_includedir}/%{name}/

%files -n python3-%{name}
%{python3_sitearch}/%{name}/

%changelog
