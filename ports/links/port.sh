#!/bin/sh
set -e

. ../common.sh

VERSION=2.25
prep http://links.twibright.com/download/links-${VERSION}.tar.gz links-${VERSION}.tar.gz links-${VERSION}

export CC=${ARCH}-pc-elf-gcc
./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
