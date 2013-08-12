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
     test_repomd.c
     testsys.c
     test_url_substitution.c
     test_util.c
     test_version.c
    )

#ADD_LIBRARY(testsys STATIC testsys.c)
#SET_TARGET_PROPERTIES(testsys PROPERTIES COMPILE_FLAGS -fPIC)

ADD_EXECUTABLE(test_main ${librepotest_SRCS})
TARGET_LINK_LIBRARIES(test_main
    librepo
    ${CHECK_LIBRARY}
    )
ADD_TEST(test_main test_main "${CMAKE_CURRENT_SOURCE_DIR}/test_data/")

# Detect nosetest version suffix
execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import sys; sys.stdout.write('%s.%s' % (sys.version_info.major, sys.version_info.minor))" OUTPUT_VARIABLE PYTHON_MAJOR_DOT_MINOR_VERSION)
set(NOSETEST_VERSION_SUFFIX "-${PYTHON_MAJOR_DOT_MINOR_VERSION}")

ADD_SUBDIRECTORY (python)
