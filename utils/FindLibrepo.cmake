# - Try to find Librepo
# Once done this will define
#  LIBREPO_FOUND - System has Librepo
#  LIBREPO_INCLUDE_DIRS - The Librepo include directories
#  LIBREPO_LIBRARIES - The libraries needed to use Librepo
#  LIBREPO_DEFINITIONS - Compiler switches required for using Librepo

find_package(PkgConfig)
pkg_check_modules(PC_LIBREPO QUIET librepo)
set(LIBREPO_DEFINITIONS ${PC_LIBREPO_CFLAGS_OTHER})

find_path(LIBREPO_INCLUDE_DIR librepo/librepo.h
          HINTS ${PC_LIBREPO_INCLUDEDIR} ${PC_LIBREPO_INCLUDE_DIRS}
          PATH_SUFFIXES libxml2 )

find_library(LIBREPO_LIBRARY NAMES repo librepo
             HINTS ${PC_LIBREPO_LIBDIR} ${PC_LIBREPO_LIBRARY_DIRS} )

set(LIBREPO_LIBRARIES ${LIBREPO_LIBRARY} )
set(LIBREPO_INCLUDE_DIRS ${LIBREPO_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LIBREPO_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Librepo  DEFAULT_MSG
                                  LIBREPO_LIBRARY LIBREPO_INCLUDE_DIR)

mark_as_advanced(LIBREPO_INCLUDE_DIR LIBREPO_LIBRARY )

