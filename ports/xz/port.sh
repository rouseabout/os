#!/bin/sh
set -e

. ../common.sh

VERSION=5.6.2
prep https://github.com/tukaani-project/xz/releases/download/v${VERSION}/xz-${VERSION}.tar.xz xz-${VERSION}.tar.xz xz-${VERSION}

./configure --host=${ARCH}-pc-elf --enable-threads=no
make -j$(nproc)
make DESTDIR=$SYSROOT install
