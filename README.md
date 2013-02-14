# librepo

librepo - A library providing C and Python (libcURL like) API for downloading
linux repository metadata and packages

## Building

### Build requires:

* check (http://check.sourceforge.net/) - in Fedora: check-devel
* expat (http://expat.sourceforge.net/) - in Fedora: expat-devel
* gcc (http://gcc.gnu.org/)
* gpgme (http://www.gnupg.org/) - in Fedora: gpgme-devel
* libcurl (http://curl.haxx.se/libcurl/) - in Fedora: libcurl-devel
* openssl (http://www.openssl.org/) - in Fedora: openssl-devel
* python (http://python.org/) - in Fedora: python2-devel

### Build from your checkout dir:

    mkdir build
    cd build/
    cmake ..
    make

## Documentation

### Build:

    cd build/
    make doc

* C documentation: `build/doc/c/html/index.html`
* Python documentation: `build/doc/python/index.html`

### Online python bindings documentation:

http://tojaj.github.com/librepo/

## Testing

All unit tests run from librepo checkout dir

### Build:
    cd build/tests
    make tests

### Run (from your checkout dir) - C unittests:

    build/tests/test_main tests/test_data/

### Run (from your checkout dir) - Python unittests:

    PYTHONPATH=`readlink -f ./build/librepo/python/` nosetests -s tests/python/tests/

