SET(pylibrepo_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})

SET (librepomodule_SRCS
     ${pylibrepo_SRCDIR}/downloader-py.c
     ${pylibrepo_SRCDIR}/exception-py.c
     ${pylibrepo_SRCDIR}/handle-py.c
     ${pylibrepo_SRCDIR}/librepomodule.c
     ${pylibrepo_SRCDIR}/logging.c
     ${pylibrepo_SRCDIR}/metadatadownloader-py.c
     ${pylibrepo_SRCDIR}/metadatatarget-py.c
     ${pylibrepo_SRCDIR}/packagedownloader-py.c
     ${pylibrepo_SRCDIR}/packagetarget-py.c
     ${pylibrepo_SRCDIR}/result-py.c
     ${pylibrepo_SRCDIR}/typeconversion.c
     ${pylibrepo_SRCDIR}/yum-py.c)

MESSAGE("Building python bindings")

EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c "
from sys import stdout
from sysconfig import get_path
path=get_path(name='platlib', vars={'platbase':'${CMAKE_INSTALL_PREFIX}'})
stdout.write(path)"
OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
INCLUDE_DIRECTORIES (${PYTHON_INCLUDE_PATH})

MESSAGE(STATUS "Python3 install dir is ${PYTHON_INSTALL_DIR}")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing")

SET (librepo_COPIES ${pylibrepo_SRCDIR}/__init__.py)
FILE(COPY ${librepo_COPIES} DESTINATION ./librepo/)

ADD_LIBRARY(_librepo SHARED ${librepomodule_SRCS})
SET_TARGET_PROPERTIES(_librepo PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(_librepo PROPERTIES LIBRARY_OUTPUT_DIRECTORY "./librepo")
TARGET_LINK_LIBRARIES(_librepo librepo)
TARGET_LINK_LIBRARIES(_librepo
                        ${CURL_LIBRARY}
                        ${PYTHON_LIBRARY}
                     )

INSTALL(FILES ${pylibrepo_SRCDIR}/__init__.py DESTINATION ${PYTHON_INSTALL_DIR}/librepo)
INSTALL(TARGETS _librepo LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/librepo)

SET(CMAKE_SKIP_RPATH TRUE)
