#!/bin/sh
set -e

. ../common.sh

VERSION=1.3.1
prep https://ftp.gnu.org/gnu/mpc/mpc-${VERSION}.tar.gz mpc-${VERSION}.tar.gz mpc-${VERSION}

./configure --host=${ARCH}-pc-elf --disable-shared
make -j$(nproc)
make DESTDIR=$SYSROOT install
