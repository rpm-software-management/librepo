SET (librepo_SRCS
     handle.c
     metalink.c
     util.c
     mirrorlist.c
     yum.c
     repomd.c
     curl.c
     checksum.c
     repoutil_yum.c
     result.c
     librepo.c
     internal_mirrorlist.c
     curltargetlist.c
     package_downloader.c
     rcodes.c
     gpg.c)

SET(librepo_HEADERS
    librepo.h
    rcodes.h
    types.h
    handle.h
    result.h
    yum.h
    util.h
    checksum.h
    repoutil_yum.h
    package_downloader.h)

ADD_LIBRARY(librepo SHARED ${librepo_SRCS})
TARGET_LINK_LIBRARIES(librepo ${EXPAT_LIBRARIES})
TARGET_LINK_LIBRARIES(librepo ${CURL_LIBRARY})
TARGET_LINK_LIBRARIES(librepo ${GPGME_VANILLA_LIBRARIES})
SET_TARGET_PROPERTIES(librepo PROPERTIES OUTPUT_NAME "repo")
SET_TARGET_PROPERTIES(librepo PROPERTIES SOVERSION 0)
#SET_TARGET_PROPERTIES(librepo PROPERTIES VERSION "0")

CONFIGURE_FILE("librepo.pc.cmake" "${CMAKE_BINARY_DIR}/librepo.pc" @ONLY)
CONFIGURE_FILE("version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/version.h" @ONLY)

IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  SET (LIB_SUFFIX "64")
ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
SET (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")

INSTALL(FILES ${librepo_HEADERS} DESTINATION include/librepo)
INSTALL(TARGETS librepo LIBRARY DESTINATION ${LIB_INSTALL_DIR})
INSTALL(FILES "${CMAKE_BINARY_DIR}/librepo.pc"
       DESTINATION "${LIB_INSTALL_DIR}/pkgconfig")

ADD_SUBDIRECTORY(python)
