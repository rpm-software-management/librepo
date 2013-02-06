SET (librepotest_SRCS
     fixtures.c
     test_checksum.c
     test_gpg.c
     test_internal_mirrorlist.c
     test_main.c
     test_repomd.c
     test_util.c
     testsys.c
    )

#ADD_LIBRARY(testsys STATIC testsys.c)
#SET_TARGET_PROPERTIES(testsys PROPERTIES COMPILE_FLAGS -fPIC)

ADD_EXECUTABLE(test_main ${librepotest_SRCS})
TARGET_LINK_LIBRARIES(test_main
    librepo
    ${CHECK_LIBRARY}
    )
ADD_TEST(test_main test_main "${CMAKE_CURRENT_SOURCE_DIR}/repos/")

ADD_SUBDIRECTORY (python)
