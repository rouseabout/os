#!/bin/sh
set -e

. ../common.sh

VERSION=2.8.9rel.1
prep https://invisible-mirror.net/archives/lynx/tarballs/lynx${VERSION}.tar.gz lynx${VERSION}.tar.gz lynx${VERSION}

export CFLAGS=-I${SYSROOT}/usr/local/include/ncurses
./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
