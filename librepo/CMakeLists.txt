LIST(APPEND librepo_SRCS
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

IF(USE_GPGME)
    LIST(APPEND librepo_SRCS gpg_gpgme.c)
ELSE(USE_GPGME)
    LIST(APPEND librepo_SRCS gpg_rpm.c)
ENDIF(USE_GPGME)

LIST(APPEND librepo_HEADERS
    checksum.h
    fastestmirror.h
    gpg.h
    ${CMAKE_CURRENT_BINARY_DIR}/handle.h
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
    ${CMAKE_CURRENT_BINARY_DIR}/util.h
    version.h
    xmlparser.h
    yum.h
    downloader.h
    ${CMAKE_CURRENT_BINARY_DIR}/downloadtarget.h)

LIST(APPEND librepo_internal_HEADERS
    downloader_internal.h
    downloadtarget_internal.h
    fastestmirror_internal.h
    gpg_internal.h
    handle_internal.h
    repoconf_internal.h
    result_internal.h
    xattr_internal.h
    xmlparser_internal.h
    yum_internal.h)

ADD_LIBRARY(librepo SHARED ${librepo_SRCS} ${librepo_HEADERS} ${librepo_internal_HEADERS})
TARGET_LINK_LIBRARIES(librepo
                        ${LIBXML2_LIBRARIES}
                        ${CURL_LIBRARY}
                        ${LIBCRYPTO_LIBRARIES}
                        ${JSONGLIB_LIBRARIES}
                        ${GLIB2_LIBRARIES}
                     )
IF (USE_GPGME)
    TARGET_LINK_LIBRARIES(librepo ${GPGME_VANILLA_LIBRARIES})
    IF (ENABLE_SELINUX)
        TARGET_LINK_LIBRARIES(librepo ${SELINUX_LIBRARIES})
    ENDIF(ENABLE_SELINUX)
ELSE(USE_GPGME)
    TARGET_LINK_LIBRARIES(librepo ${RPM_LIBRARIES})
ENDIF (USE_GPGME)

IF (WITH_ZCHUNK)
    TARGET_LINK_LIBRARIES(librepo ${ZCHUNKLIB_LIBRARIES})
    SET(PKGCONF_DEPENDENCY_ZCK "zck")
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
