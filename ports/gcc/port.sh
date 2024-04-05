#!/bin/sh
set -e

. ../common.sh

VERSION=13.2.0
prep https://ftp.gnu.org/gnu/gcc/gcc-${VERSION}/gcc-${VERSION}.tar.xz gcc-${VERSION}.tar.xz gcc-${VERSION}

export ac_cv_c_bigendian=no
./configure --host=${ARCH}-pc-elf --target=${ARCH}-pc-elf --with-sysroot=/ --with-build-sysroot=$SYSROOT --enable-languages=c --disable-lto --disable-nls
make -j$(nproc) all-gcc
make -j$(nproc) all-target-libgcc
make DESTDIR=$SYSROOT install-gcc install-target-libgcc
