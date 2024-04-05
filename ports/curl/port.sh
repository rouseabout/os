#!/bin/sh
set -e

. ../common.sh

VERSION=7.82.0
prep https://curl.se/download/curl-${VERSION}.tar.xz curl-${VERSION}.tar.xz curl-${VERSION}

./configure --host=${ARCH}-pc-elf --without-ssl --disable-pthreads --disable-threaded-resolver
make -j$(nproc)
make DESTDIR=$SYSROOT install
