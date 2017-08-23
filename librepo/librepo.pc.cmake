prefix=@CMAKE_INSTALL_PREFIX@
libdir=@CMAKE_INSTALL_FULL_LIBDIR@
includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@

Name: librepo
Description: Repodata downloading library.
Version: @VERSION@
Requires: glib-2.0
Requires.private: libcurl openssl
Libs: -L${libdir} -lrepo
Libs.private: -lexpat -gpgme -gpg-error
Cflags: -I${includedir} -D_FILE_OFFSET_BITS=64
