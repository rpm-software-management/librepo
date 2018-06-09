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
     test_repo_zck.c
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
CONFIGURE_FILE("run_tests.sh.in"  "${CMAKE_BINARY_DIR}/tests/run_tests.sh")
ADD_TEST(test_main run_tests.sh)


ADD_SUBDIRECTORY (python)
