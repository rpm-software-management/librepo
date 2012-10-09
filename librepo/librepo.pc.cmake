prefix=@CMAKE_INSTALL_PREFIX@
libdir=@LIB_INSTALL_DIR@
includedir=@CMAKE_INSTALL_PREFIX@/include

Name: librepo
Description: Repodata downloading library.
Version: @VERSION@
Requires.private: libcurl
Libs: -L${libdir} -lrepo
Libs.private: -lcurl -lexpat
Cflags: -I${includedir} -D_FILE_OFFSET_BITS=64
