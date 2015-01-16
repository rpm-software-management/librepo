# librepo

librepo - A library providing C and Python (libcURL like) API for downloading
linux repository metadata and packages

## Building

### Build requires:

Fedora/Ubuntu name

* check (http://check.sourceforge.net/) - check-devel/check
* cmake (http://www.cmake.org/) - cmake/cmake
* expat (http://expat.sourceforge.net/) - expat-devel/libexpat1-dev
* gcc (http://gcc.gnu.org/) - gcc/gcc
* glib2 (http://developer.gnome.org/glib/) - glib2-devel/libglib2.0-dev
* gpgme (http://www.gnupg.org/) - gpgme-devel/libgpgme11-dev
* libattr (http://www.bestbits.at/acl/) - libattr-devel/libattr1-dev
* libcurl (http://curl.haxx.se/libcurl/) - libcurl-devel/libcurl4-openssl-dev
* openssl (http://www.openssl.org/) - openssl-devel/libssl-dev
* python (http://python.org/) - python2-devel/libpython2.7-dev (python3-devel/libpython3-dev)
* **Test requires:** pygpgme (https://pypi.python.org/pypi/pygpgme/0.1) - pygpgme/python-gpgme (python3-pygpgme/python3-gpgme)
* **Test requires:** python-flask (http://flask.pocoo.org/) - python-flask/python-flask
* **Test requires:** python-nose (https://nose.readthedocs.org/) - python-nose/python-nose (python3-nose)
* **Test requires:** pyxattr (https://github.com/xattr/xattr) - /pyxattr (python3-pyxattr)

### Build from your checkout dir (Python 2 bindings):

    mkdir build
    cd build/
    cmake ..
    make

### Build from you checkout dir (Python 3 bindings):

    mkdir build
    cd build/
    cmake -DPYTHON_DESIRED="3" ..
    make

### Build with debug flags:

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

## Python 3 notes:

Support for Python 3 is experimental.

Python unittests are not fully working due to missing python3 support in
Flask module.

Flask python3 support rely on python3 support in:
* [Werkzeug module](http://werkzeug.pocoo.org/docs/python3/) (Currently - 13.8.2013 - Highly experimental)
* [Jinja2 module](http://jinja.pocoo.org/docs/intro/) (Done)
* [itsdangerous module](https://github.com/mitsuhiko/itsdangerous) (Done)

## Testing

All unit tests run from librepo checkout dir

### Run both (C & Python) tests via makefile:
    make test

### Run (from your checkout dir) - C unittests:

    build/tests/test_main tests/test_data/

Available params:

* ``-v`` Run tests verbosely (Show Librepo debug messages)
* ``-d`` Run download tests (This tests need internet connection)

To check memoryleaks:

Add this line to your ``~/.bashrc`` file

    alias gvalgrind='G_SLICE=always-malloc G_DEBUG=gc-friendly valgrind'

And then run:

    CK_FORK=no gvalgrind --leak-check=full build/tests/test_main tests/test_data/

Supress known still_reachable memory:

    CK_FORK=no gvalgrind --leak-check=full --suppressions=still_reachable.supp build/tests/test_main tests/test_data/

Note: .valgrindrc file is present in checkoutdir, this file contains the settings:
`--memcheck:leak-check=full --suppressions=./valgrind.supp`

### Run (from your checkout dir) - Python 2 unittests:

    PYTHONPATH=`readlink -f ./build/librepo/python/python2/` nosetests -s -v tests/python/tests/

Example of run only one specific test: ``PYTHONPATH=`readlink -f ./build/librepo/python/python2/` nosetests -s -v tests/python/tests/test_yum_repo_downloading.py:TestCaseYumRepoDownloading.test_download_and_update_repo_01``

### Run (from your checkout dir) - Python 3 unittests:

    PYTHONPATH=`readlink -f ./build/librepo/python/python3/` nosetests-3.3 -s -v tests/python/tests/

## Links

* [Red Hat Bugzilla](https://bugzilla.redhat.com/buglist.cgi?query_format=advanced&bug_status=NEW&bug_status=ASSIGNED&bug_status=MODIFIED&bug_status=VERIFIED&component=librepo)

* [Fedora spec file](http://pkgs.fedoraproject.org/cgit/librepo.git/tree/librepo.spec)
