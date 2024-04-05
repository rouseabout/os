#!/bin/sh
set -e

. ../common.sh

VERSION=3450200
prep https://www.sqlite.org/2024/sqlite-autoconf-${VERSION}.tar.gz sqlite-autoconf-${VERSION}.tar.gz sqlite-autoconf-${VERSION}

./configure --host=${ARCH}-pc-elf --disable-threadsafe
make -j$(nproc)
make DESTDIR=$SYSROOT install
