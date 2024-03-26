#!/bin/sh
set -e

. ../common.sh

VERSION=1.3
prep http://zlib.net/zlib-${VERSION}.tar.xz zlib-${VERSION}.tar.xz zlib-${VERSION}

CC=i686-pc-elf-gcc AR=i686-pc-elf-ar ./configure --static --prefix=$SYSROOT/usr
make -j$(nproc)
make install
