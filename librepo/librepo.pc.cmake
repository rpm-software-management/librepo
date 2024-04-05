prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_FULL_LIBDIR@
includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@

Name: librepo
Description: Repodata downloading library.
Version: @VERSION@
Requires: glib-2.0
Requires.private: libcurl openssl libxml-2.0
Libs: -L${libdir} -lrepo
Libs.private: -lgpgme -lgpg-error
Cflags: -I${includedir} @PKGCONF_CFLAGS_ZCHUNK@ -D_FILE_OFFSET_BITS=64
