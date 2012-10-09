FIND_PACKAGE (PythonLibs)
FIND_PACKAGE (PythonInterp REQUIRED)
EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c "from sys import stdout; from distutils import sysconfig; stdout.write(sysconfig.get_python_lib(True))" OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
INCLUDE_DIRECTORIES (${PYTHON_INCLUDE_PATH})

MESSAGE(STATUS "Python install dir is ${PYTHON_INSTALL_DIR}")

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing")

SET (librepo_COPIES __init__.py)
FILE(COPY ${librepo_COPIES} DESTINATION ./librepo/)

SET (librepomodule_SRCS
     exception-py.c
     librepomodule.c)

ADD_LIBRARY(_librepomodule SHARED ${librepomodule_SRCS})
SET_TARGET_PROPERTIES(_librepomodule PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(_librepomodule PROPERTIES LIBRARY_OUTPUT_DIRECTORY "./librepo")
TARGET_LINK_LIBRARIES(_librepomodule librepo)
TARGET_LINK_LIBRARIES(_librepomodule ${EXPAT_LIBRARIES})
TARGET_LINK_LIBRARIES(_librepomodule ${CURL_LIBRARY})
TARGET_LINK_LIBRARIES(_librepomodule ${PYTHON_LIBRARY})

#INSTALL(FILES __init__.py DESTINATION ${PYTHON_INSTALL_DIR}/librepo)
#INSTALL(TARGETS _librepomodule LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/librepo)
