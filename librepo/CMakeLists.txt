SET (librepo_SRCS
     checksum.c
     curl.c
     curltargetlist.c
     gpg.c
     handle.c
     internal_mirrorlist.c
     librepo.c
     metalink.c
     mirrorlist.c
     package_downloader.c
     rcodes.c
     repomd.c
     repoutil_yum.c
     result.c
     util.c
     yum.c)

SET(librepo_HEADERS
    checksum.h
    gpg.h
    handle.h
    librepo.h
    package_downloader.h
    rcodes.h
    repomd.h
    repoutil_yum.h
    result.h
    types.h
    util.h
    version.h
    yum.h)

ADD_LIBRARY(librepo SHARED ${librepo_SRCS})
TARGET_LINK_LIBRARIES(librepo
                        ${EXPAT_LIBRARY}
                        ${CURL_LIBRARY}
                        ${GPGME_VANILLA_LIBRARIES}
                     )
SET_TARGET_PROPERTIES(librepo PROPERTIES OUTPUT_NAME "repo")
SET_TARGET_PROPERTIES(librepo PROPERTIES SOVERSION 0)
#SET_TARGET_PROPERTIES(librepo PROPERTIES VERSION "0")

CONFIGURE_FILE("librepo.pc.cmake" "${CMAKE_BINARY_DIR}/librepo.pc" @ONLY)
CONFIGURE_FILE("version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/version.h" @ONLY)
CONFIGURE_FILE("setup.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/setup.h" @ONLY)

IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  SET (LIB_SUFFIX "64")
ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")
SET (LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}")

INSTALL(FILES ${librepo_HEADERS} DESTINATION include/librepo)
INSTALL(TARGETS librepo LIBRARY DESTINATION ${LIB_INSTALL_DIR})
INSTALL(FILES "${CMAKE_BINARY_DIR}/librepo.pc"
       DESTINATION "${LIB_INSTALL_DIR}/pkgconfig")

ADD_SUBDIRECTORY(python)
