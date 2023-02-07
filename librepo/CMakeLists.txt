SET (librepo_SRCS
     checksum.c
     downloader.c
     downloadtarget.c
     fastestmirror.c
     gpg.c
     handle.c
     lrmirrorlist.c
     metalink.c
     metadata_downloader.c
     mirrorlist.c
     package_downloader.c
     rcodes.c
     repoconf.c
     repomd.c
     repoutil_yum.c
     result.c
     url_substitution.c
     util.c
     xmlparser.c
     yum.c)

SET(librepo_HEADERS
    checksum.h
    fastestmirror.h
    gpg.h
    handle.h
    librepo.h
    metadata_downloader.h
    metalink.h
    mirrorlist.h
    package_downloader.h
    rcodes.h
    repoconf.h
    repomd.h
    repoutil_yum.h
    result.h
    types.h
    url_substitution.h
    util.h
    version.h
    xmlparser.h
    yum.h
    downloader.h
    downloadtarget.h)

ADD_LIBRARY(librepo SHARED ${librepo_SRCS})
TARGET_LINK_LIBRARIES(librepo
                        ${LIBXML2_LIBRARIES}
                        ${CURL_LIBRARY}
                        ${LIBCRYPTO_LIBRARIES}
                        ${GPGME_VANILLA_LIBRARIES}
                        ${GLIB2_LIBRARIES}
                     )
IF (WITH_ZCHUNK)
    TARGET_LINK_LIBRARIES(librepo ${ZCHUNKLIB_LIBRARIES})
ENDIF (WITH_ZCHUNK)

SET_TARGET_PROPERTIES(librepo PROPERTIES OUTPUT_NAME "repo")
SET_TARGET_PROPERTIES(librepo PROPERTIES SOVERSION 0)
#SET_TARGET_PROPERTIES(librepo PROPERTIES VERSION "0")

CONFIGURE_FILE("librepo.pc.cmake" "${CMAKE_BINARY_DIR}/librepo.pc" @ONLY)
CONFIGURE_FILE("version.h.in" "${CMAKE_CURRENT_SOURCE_DIR}/version.h" @ONLY)

IF (CMAKE_SIZEOF_VOID_P MATCHES "8")
  SET (LIB_SUFFIX "64")
ENDIF (CMAKE_SIZEOF_VOID_P MATCHES "8")

INSTALL(FILES ${librepo_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/librepo)
INSTALL(TARGETS librepo LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(FILES "${CMAKE_BINARY_DIR}/librepo.pc"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")

IF (ENABLE_PYTHON)
    ADD_SUBDIRECTORY(python)
ENDIF (ENABLE_PYTHON)
