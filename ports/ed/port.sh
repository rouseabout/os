#!/bin/sh
set -e

. ../common.sh

VERSION=1.4
prep https://ftp.gnu.org/gnu/ed/ed-${VERSION}.tar.gz ed-${VERSION}.tar.gz ed-${VERSION}

CC=${ARCH}-pc-elf-gcc ./configure
make -j$(nproc)
make DESTDIR=$SYSROOT install
