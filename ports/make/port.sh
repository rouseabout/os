#!/bin/sh
set -e

. ../common.sh

VERSION=4.3
prep https://ftp.gnu.org/gnu/make/make-${VERSION}.tar.gz make-${VERSION}.tar.gz make-${VERSION}

CFLAGS=-DNO_ARCHIVES ./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
