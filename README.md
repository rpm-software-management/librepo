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
* **Test requires:** pygpgme (https://pypi.python.org/pypi/pygpgme/0.1) - in Fedora: pygpgme
* **Test requires:** python-flask (http://flask.pocoo.org/) - in Fedora: python-flask
* **Test requires:** python-nose (https://nose.readthedocs.org/) - in Fedora: python-nose

### Build from your checkout dir:

    mkdir build
    cd build/
    cmake ..
    make

### Build with debug messages:

    mkdir build
    cd build/
    cmake -DCMAKE_BUILD_TYPE="DEBUG" ..
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

### Run both (C & Python) tests via makefile:
    make test

### Run (from your checkout dir) - C unittests:

    build/tests/test_main tests/test_data/

To check memoryleaks:

    CK_FORK=no valgrind --leak-check=full build/tests/test_main tests/test_data/

Supress known still_reachable memory:

    CK_FORK=no valgrind --leak-check=full --suppressions=still_reachable.supp build/tests/test_main tests/test_data/

Note: .valgrindrc file is present in checkoutdir, this file contains the settings:
`--memcheck:leak-check=full --suppressions=./valgrind.supp`

### Run (from your checkout dir) - Python unittests:

    PYTHONPATH=`readlink -f ./build/librepo/python/` nosetests -s tests/python/tests/

Example of run only one specific test: ``PYTHONPATH=`readlink -f ./build/librepo/python/` nosetests -s tests/python/tests/test_yum_repo_downloading.py:TestCaseYumRepoDownloading.test_download_and_update_repo_01``

## Links

* [Red Hat Bugzilla](https://bugzilla.redhat.com/buglist.cgi?query_format=advanced&bug_status=NEW&bug_status=ASSIGNED&bug_status=MODIFIED&bug_status=VERIFIED&component=librepo)
