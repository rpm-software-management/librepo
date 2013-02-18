# Utils - Some useful scripts and other stuff

## FindLibrepo.cmake

CMake module for find Librepo library.
[More info](http://www.cmake.org/Wiki/CMake:How_To_Find_Libraries#Using_external_libraries_that_CMake_doesn.27t_yet_have_modules_for, "CMake:How To Find Libraries")

## make_rpm.sh

**Usage (from root project dir):**

    utils/make_rpm.sh . [git_revision]

Generate tarbal and make rpm for specified git revision.
Spec file librepo.spec must be available in root project dir.

## make_tarball.sh

**Usage (from root project dir):**

    utils/make_tarball.sh [git_revision]

Generate tarbal for specified git revision.


