%bcond_without python2
%if 0%{?rhel} && 0%{?rhel} <= 7
# Do not build bindings for python3 for RHEL <= 7
%bcond_with python3
# python-flask is not in RHEL7
%bcond_with tests
# platform-python is not in RHEL7
%bcond_with platform_python
%else
%bcond_without python3
%bcond_without platform_python
%bcond_without tests
%endif

Name:           librepo
Version:        1.8.1
Release:        1%{?dist}
Summary:        Repodata downloading library

License:        LGPLv2+
URL:            https://github.com/rpm-software-management/librepo
Source0:        %{url}/archive/%{version}/%{name}-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc
BuildRequires:  check-devel
BuildRequires:  doxygen
BuildRequires:  expat-devel
BuildRequires:  glib2-devel >= 2.26.0
BuildRequires:  gpgme-devel
BuildRequires:  libattr-devel
BuildRequires:  libcurl-devel >= 7.19.0
BuildRequires:  openssl-devel

%description
A library providing C and Python (libcURL like) API to downloading repository
metadata.

%package devel
Summary:        Repodata downloading library
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
Development files for librepo.

%if %{with python2}
%package -n python2-%{name}
Summary:        Python bindings for the librepo library
%{?python_provide:%python_provide python2-%{name}}
BuildRequires:  pygpgme
BuildRequires:  python2-devel
%if %{with tests}
BuildRequires:  python-flask
BuildRequires:  python-nose
%endif
BuildRequires:  python-sphinx
BuildRequires:  pyxattr
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n python2-%{name}
Python 2 bindings for the librepo library.
%endif # with python2

%if %{with python3}
%package -n python3-%{name}
Summary:        Python 3 bindings for the librepo library
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

%description -n python3-%{name}
Python 3 bindings for the librepo library.
%endif

%if %{with platform_python}
%package -n platform-python-%{name}
Summary:        Platform Python bindings for the librepo library
BuildRequires:  platform-python-devel
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description -n platform-python-%{name}
Python 3 bindings for the librepo library.
%endif # with platform_python

%prep
%autosetup -p1

mkdir build build-py3 build-platpy

%build

%if %{with python2}
pushd build
  %cmake ..
  %make_build
popd
%endif # with python2

%if %{with python3}
pushd build-py3
  %cmake -DPYTHON_DESIRED:str=3 ..
  %make_build
popd
%endif

%if %{with platform_python}
pushd build-platpy

  # librepo's CMakeLists override CMake's override mechanism!
  # Bring it back.
  sed '/unset(PYTHON_[^)]*)/d' -i ../librepo/python/python3/CMakeLists.txt

  export python_so=%{_libdir}/`%{__platform_python} -c 'import sysconfig; print(sysconfig.get_config_var("LDLIBRARY"))'`
  export python_include=`%{__platform_python} -c 'import sysconfig; print(sysconfig.get_path("include"))'`

  %cmake \
    -DPYTHON_EXECUTABLE:FILEPATH=%{__platform_python} \
    -DPYTHON_LIBRARY=$python_so \
    -DPYTHON_INCLUDE_DIR=$python_include \
    -DPYTHON_DESIRED:str=3 \
    ..
  %make_build
popd
%endif # with platform_python

%if %{with tests}
%check
%if %{with python2}
pushd build
  #ctest -VV
  make ARGS="-V" test
popd
%endif # with python2

%if %{with python3}
pushd build-py3
  #ctest -VV
  make ARGS="-V" test
popd
%endif # with python3

%if %{with platform_python}
pushd build-platpy
  #ctest -VV

  # Test suite requires the "nosetests" binary
  #make ARGS="-V" test
popd
%endif # with platform_python
%endif # with tests

%install

%if %{with platform_python}
pushd build-platpy
  %make_install
popd
%endif # with platform_python

%if %{with python2}
pushd build
  %make_install
popd
%endif # with python2

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

%if %{with python2}
%files -n python2-%{name}
%{python2_sitearch}/%{name}/
%endif # with python2

%if %{with python3}
%files -n python3-%{name}
%{python3_sitearch}/%{name}/
%endif

%if %{with platform_python}
%files -n platform-python-%{name}
%{platform_python_sitearch}/%{name}/
%endif # with platform_python

%changelog
