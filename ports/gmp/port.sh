#!/bin/sh
set -e

. ../common.sh

VERSION=6.3.0
prep https://ftpmirror.gnu.org/gnu/gmp/gmp-${VERSION}.tar.xz gmp-${VERSION}.tar.xz gmp-${VERSION}

./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
