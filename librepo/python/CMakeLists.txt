SET(pylibrepo_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})

SET (librepomodule_SRCS
     ${pylibrepo_SRCDIR}/downloader-py.c
     ${pylibrepo_SRCDIR}/exception-py.c
     ${pylibrepo_SRCDIR}/handle-py.c
     ${pylibrepo_SRCDIR}/librepomodule.c
     ${pylibrepo_SRCDIR}/logging.c
     ${pylibrepo_SRCDIR}/packagedownloader-py.c
     ${pylibrepo_SRCDIR}/packagetarget-py.c
     ${pylibrepo_SRCDIR}/result-py.c
     ${pylibrepo_SRCDIR}/typeconversion.c
     ${pylibrepo_SRCDIR}/yum-py.c)

if (${PYTHON_DESIRED} STREQUAL "2")
    message("Building for python2")
    add_subdirectory(python2)
else()
    message("Building for python3")
    add_subdirectory(python3)
endif()
