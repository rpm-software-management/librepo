SET (librepotest_SRCS
     fixtures.c
     test_checksum.c
     test_downloader.c
     test_gpg.c
     test_handle.c
     test_lrmirrorlist.c
     test_main.c
     test_metalink.c
     test_mirrorlist.c
     test_package_downloader.c
     test_repoconf.c
     test_repomd.c
     testsys.c
     test_url_substitution.c
     test_util.c
     test_util_deprecated.c
     test_version.c
    )

#ADD_LIBRARY(testsys STATIC testsys.c)
#SET_TARGET_PROPERTIES(testsys PROPERTIES COMPILE_FLAGS -fPIC)

ADD_EXECUTABLE(test_main ${librepotest_SRCS})
TARGET_LINK_LIBRARIES(test_main
    librepo
    ${CHECK_LIBRARY}
    )
CONFIGURE_FILE("run_tests.sh.in"  "${CMAKE_BINARY_DIR}/tests/run_tests.sh")
ADD_TEST(test_main run_tests.sh)

# Detect nosetest version suffix
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import sys; sys.stdout.write('%s.%s' % (sys.version_info[0], sys.version_info[1]))" OUTPUT_VARIABLE PYTHON_MAJOR_DOT_MINOR_VERSION)
set(NOSETEST_VERSION_SUFFIX "-${PYTHON_MAJOR_DOT_MINOR_VERSION}")

execute_process(COMMAND which nosetests${NOSETEST_VERSION_SUFFIX}
                OUTPUT_QUIET ERROR_QUIET
                RESULT_VARIABLE NOSE_CHECK_RESULT)
IF (NOT NOSE_CHECK_RESULT STREQUAL "0")
    MESSAGE("Command 'nosetests${NOSETEST_VERSION_SUFFIX}' doesn't exist! Using only 'nosetests' instead")
    SET(NOSETEST_VERSION_SUFFIX "")
ENDIF()


ADD_SUBDIRECTORY (python)
