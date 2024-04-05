#!/bin/sh
set -e

. ../common.sh

VERSION=1.16
prep https://dev.yorhel.nl/download/ncdu-${VERSION}.tar.gz ncdu-${VERSION}.tar.gz ncdu-${VERSION}

export CFLAGS=-I${SYSROOT}/usr/local/include/ncurses
./configure --host=${ARCH}-pc-elf --with-ncurses
make -j$(nproc)
make DESTDIR=$SYSROOT install
