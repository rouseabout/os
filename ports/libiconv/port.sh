#!/bin/sh
set -e

. ../common.sh

VERSION=1.17
prep https://ftp.gnu.org/pub/gnu/libiconv/libiconv-${VERSION}.tar.gz libiconv-${VERSION}.tar.gz libiconv-${VERSION}

./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
