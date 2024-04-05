#!/bin/sh
set -e

. ../common.sh

VERSION=2.41
prep https://ftp.gnu.org/gnu/binutils/binutils-${VERSION}.tar.xz binutils-${VERSION}.tar.xz binutils-${VERSION}

./configure --host=${ARCH}-pc-elf --target=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
