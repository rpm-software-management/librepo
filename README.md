# librepo

librepo - A library providing C and Python (libcURL like) API to downloading repository

## Building

### Build requires:

* gcc (http://gcc.gnu.org/)
* libcurl (http://curl.haxx.se/libcurl/) - in Fedora: libcurl-devel
* check (http://check.sourceforge.net/) - in Fedora: check-devel
* expat (http://expat.sourceforge.net/) - in Fedora: expat-devel
* python (http://python.org/) - in Fedora: python2-devel

### Build from your checkout dir:

    mkdir build
    cd build/
    cmake ..
    make

## Unittests:

    All unit tests run from librepo checkout dir

### Build:
    cd build/tests
    make tests

### Run (from your checkout dir) - C unittests:

    build/tests/test_main tests/repos/

### Run (from your checkout dir) - Python unittests:

    PYTHONPATH=`readlink -f ./build/librepo/python/` nosetests -s tests/python/tests/

