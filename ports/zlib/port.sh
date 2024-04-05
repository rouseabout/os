#!/bin/sh
set -e

. ../common.sh

VERSION=1.3
prep http://zlib.net/zlib-${VERSION}.tar.xz zlib-${VERSION}.tar.xz zlib-${VERSION}

CC=${ARCH}-pc-elf-gcc AR=${ARCH}-pc-elf-ar ./configure --static --prefix=$SYSROOT/usr
make -j$(nproc)
make install
