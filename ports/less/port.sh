#!/bin/sh
set -e

. ../common.sh

VERSION=590
prep https://ftp.gnu.org/gnu/less/less-${VERSION}.tar.gz less-${VERSION}.tar.gz less-${VERSION}

./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
