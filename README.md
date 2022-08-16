# librepo

librepo - A library providing C and Python (libcURL like) API for downloading
linux repository metadata and packages

## Building

### Build requires:

Fedora/Ubuntu name

* check (http://check.sourceforge.net/) - check-devel/check
* cmake (http://www.cmake.org/) - cmake/cmake
* gcc (http://gcc.gnu.org/) - gcc/gcc
* glib2 (http://developer.gnome.org/glib/) - glib2-devel/libglib2.0-dev
* gpgme (http://www.gnupg.org/) - gpgme-devel/libgpgme11-dev
* libattr (https://savannah.nongnu.org/projects/attr) - libattr-devel/libattr1-dev
* libcurl (http://curl.haxx.se/libcurl/) - libcurl-devel/libcurl4-openssl-dev
* openssl (http://www.openssl.org/) - openssl-devel/libssl-dev
* python (http://python.org/) - python3-devel/libpython3-dev
* **Test requires:** pygpgme (https://pypi.python.org/pypi/pygpgme/0.1) - python3-pygpgme/python3-gpgme
* **Test requires:** python3-pyxattr (https://github.com/xattr/xattr) - python3-pyxattr/python3-pyxattr

### Build from your checkout dir:

    mkdir build
    cd build/
    cmake ..
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

https://rpm-software-management.github.io/librepo/

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

Suppress known still_reachable memory:

    CK_FORK=no gvalgrind --leak-check=full --suppressions=still_reachable.supp build/tests/test_main tests/test_data/

Note: .valgrindrc file is present in checkoutdir, this file contains the settings:
`--memcheck:leak-check=full --suppressions=./valgrind.supp`

### Run (from your checkout dir) - Python 3 unittests:

    PYTHONPATH=`readlink -f ./build/librepo/python/` python3 -m unittest discover -b -s tests/python

## Contribution

Here's the most direct way to get your work merged into the project.

1. Fork the project
1. Clone down your fork
1. Implement your feature or bug fix and commit changes
1. If the change fixes a bug at [Red Hat bugzilla](https://bugzilla.redhat.com/), or if it is important to the end user, add the following block to the commit message:

       = changelog =
       msg:           message to be included in the changelog
       type:          one of: bugfix/enhancement/security (this field is required when message is present)
       resolves:      URLs to bugs or issues resolved by this commit (can be specified multiple times)
       related:       URLs to any related bugs or issues (can be specified multiple times)

   * For example::

         = changelog =
         msg: Decode package URL when using for local filename
         type: bugfix
         resolves: https://bugzilla.redhat.com/show_bug.cgi?id=1817130

   * For your convenience, you can also use git commit template by running the following command in the top-level directory of this project:

         git config commit.template ./.git-commit-template

1. In a separate commit, add your name into the [authors file](https://github.com/rpm-software-management/librepo/blob/master/AUTHORS) as a reward for your generosity
1. Push the branch to your fork
1. Send a pull request for your branch

## Links

* [Red Hat Bugzilla](https://bugzilla.redhat.com/buglist.cgi?query_format=advanced&bug_status=NEW&bug_status=ASSIGNED&bug_status=MODIFIED&bug_status=VERIFIED&component=librepo)

* [Fedora spec file](http://pkgs.fedoraproject.org/cgit/librepo.git/tree/librepo.spec)
