#!/bin/sh
set -e

. ../common.sh

VERSION=4.9.0
prep https://ftp.gnu.org/gnu/findutils/findutils-${VERSION}.tar.xz findutils-${VERSION}.tar.xz findutils-${VERSION}

./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
