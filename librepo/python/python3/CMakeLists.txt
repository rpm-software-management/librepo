SET(Python_ADDITIONAL_VERSIONS 3.3)
message("--- ${PYTHON_LIBRARY}")
message("--- ${PYTHON_INCLUDE_DIR}")
unset(PYTHON_LIBRARY)
unset(PYTHON_INCLUDE_DIR)
unset(PYTHON_LIBRARY CACHE)
unset(PYTHON_INCLUDE_DIR CACHE)
message("--- ${PYTHON_LIBRARY}")
message("--- ${PYTHON_INCLUDE_DIR}")

FIND_PACKAGE(PythonLibs 3.0)
FIND_PACKAGE(PythonInterp 3.0 REQUIRED)
EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c "from sys import stdout; from distutils import sysconfig; stdout.write(sysconfig.get_python_lib(True))" OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
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
                        ${EXPAT_LIBRARIES}
                        ${CURL_LIBRARY}
                        ${PYTHON_LIBRARY}
                     )

INSTALL(FILES ${pylibrepo_SRCDIR}/__init__.py DESTINATION ${PYTHON_INSTALL_DIR}/librepo)
INSTALL(TARGETS _librepo LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/librepo)
