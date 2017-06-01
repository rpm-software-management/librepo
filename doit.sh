#!/bin/sh

rm -rf build

mkdir build
cd build
cmake \
        -DCMAKE_C_FLAGS:STRING="-I/opt/local/include" \
        -DCMAKE_C_FLAGS_RELEASE:STRING="-DNDEBUG" \
        -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
        -DCMAKE_INSTALL_PREFIX:PATH=/opt/local \
        -DINCLUDE_INSTALL_DIR:PATH=/opt/local/include \
        -DLIB_INSTALL_DIR:PATH=/opt/local/lib \
        -DBUILD_SHARED_LIBS:BOOL=ON \
    -DPYTHON_DESIRED="2" \
    -DMACOSX="1" \
	../

make && make test && sudo make install

otool -L /opt/local/lib/librepo.0.dylib
