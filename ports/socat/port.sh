#!/bin/sh
set -e

. ../common.sh

VERSION=1.8.0.0
prep http://www.dest-unreach.org/socat/download/socat-${VERSION}.tar.gz socat-${VERSION}.tar.gz socat-${VERSION}

./configure --host=${ARCH}-pc-elf --disable-posixmq
make -j$(nproc)
make DESTDIR=$SYSROOT install
