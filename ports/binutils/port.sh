#!/bin/sh
set -e

. ../common.sh

VERSION=2.41
prep https://ftp.gnu.org/gnu/binutils/binutils-${VERSION}.tar.xz binutils-${VERSION}.tar.xz binutils-${VERSION}

./configure --host=i686-pc-elf --target=i686-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
