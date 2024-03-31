#!/bin/sh
set -e

. ../common.sh

VERSION=3.7.2
prep https://www.libarchive.org/downloads/libarchive-${VERSION}.tar.xz libarchive-${VERSION}.tar.xz libarchive-${VERSION}

export CFLAGS=-DFNM_CASEFOLD=0
./configure --host=i686-pc-elf --without-xml2
make -j$(nproc)
make DESTDIR=$SYSROOT install
