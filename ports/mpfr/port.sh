#!/bin/sh
set -e

. ../common.sh

VERSION=4.2.1
prep https://www.mpfr.org/mpfr-current/mpfr-${VERSION}.tar.xz mpfr-${VERSION}.tar.xz mpfr-${VERSION}

./configure CC=i686-pc-elf-gcc --target=i686-pc-elf --host=i686-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
