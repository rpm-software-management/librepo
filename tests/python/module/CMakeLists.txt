FIND_PACKAGE (PythonLibs)
#FIND_PACKAGE (PythonInstDir REQUIRED)
EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE} -c "from sys import stdout; from distutils import sysconfig; stdout.write(sysconfig.get_python_lib(True))" OUTPUT_VARIABLE PYTHON_INSTALL_DIR)
INCLUDE_DIRECTORIES (${PYTHON_INCLUDE_PATH})

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-strict-aliasing")
set (CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-strict-aliasing")

SET (testmodule_SRCS
     librepo_testmodule.c)

ADD_LIBRARY(testmodule SHARED ${testmodule_SRCS})
SET_TARGET_PROPERTIES(testmodule PROPERTIES PREFIX "")
SET_TARGET_PROPERTIES(testmodule PROPERTIES OUTPUT_NAME "_librepo_testmodule")
TARGET_LINK_LIBRARIES(testmodule _librepomodule
				 #testsys
				 ${PYTHON_LIBRARY})

# copy the whole package under librepo/test so 'import librepo.test' works as expected:
SET (pytest_COPIES __init__.py)
SET (TMODULE_TARGET ${CMAKE_BINARY_DIR}/librepo/python/librepo/test)
FILE(COPY ${pytest_COPIES} DESTINATION ${TMODULE_TARGET})
ADD_CUSTOM_COMMAND(TARGET testmodule
		   POST_BUILD
		   COMMAND cp $<TARGET_FILE:testmodule> ${TMODULE_TARGET})

INSTALL(FILES __init__.py DESTINATION ${PYTHON_INSTALL_DIR}/librepo/test)
INSTALL(TARGETS testmodule LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/librepo/test)
