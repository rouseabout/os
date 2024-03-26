#!/bin/sh
set -e

. ../common.sh

VERSION=4.9.0
prep https://ftp.gnu.org/gnu/findutils/findutils-${VERSION}.tar.xz findutils-${VERSION}.tar.xz findutils-${VERSION}

./configure --host=i686-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
