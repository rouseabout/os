#!/bin/sh
set -e

. ../common.sh

VERSION=6.4
prep https://invisible-mirror.net/archives/ncurses/ncurses-${VERSION}.tar.gz ncurses-${VERSION}.tar.gz ncurses-${VERSION}

./configure --host=${ARCH}-pc-elf --without-ada --without-cxx --with-termlib --disable-database --with-fallbacks=vt100
make -j$(nproc)
make DESTDIR=$SYSROOT install
