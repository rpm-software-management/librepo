SET (librepo_SRCS
     handle.c
     metalink.c
     util.c
     mirrorlist.c
     yum.c
     repomd.c
     curl.c
     checksum.c
     repoutil_yum.c)

SET(headers
    librepo.h
    metalink.h
    util.h
    mirrorlist.h
    yum.h
    repomd.h
    curl.h
    checksum.h
    setup.h
    repoutil_yum.h)

ADD_LIBRARY(librepo SHARED ${librepo_SRCS})
TARGET_LINK_LIBRARIES(librepo ${EXPAT_LIBRARIES})
TARGET_LINK_LIBRARIES(librepo ${CURL_LIBRARY})

SET_TARGET_PROPERTIES(librepo PROPERTIES
                      OUTPUT_NAME "repo"
                      SOVERSION "0"
                      VERSION "0")

#ADD_EXECUTABLE(createrepo_c createrepo_c.c cmd_parser.c)
#TARGET_LINK_LIBRARIES(createrepo_c
#                      libcreaterepo_c
#                      ${GLIB2_LIBRARIES}
#                      ${GTHREAD2_LIBRARIES})

ADD_SUBDIRECTORY(python)
