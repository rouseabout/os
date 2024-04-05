#!/bin/sh
set -e

. ../common.sh

VERSION=4.2.1
prep https://www.mpfr.org/mpfr-current/mpfr-${VERSION}.tar.xz mpfr-${VERSION}.tar.xz mpfr-${VERSION}

./configure CC=${ARCH}-pc-elf-gcc --target=${ARCH}-pc-elf --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
