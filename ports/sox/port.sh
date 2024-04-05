#!/bin/sh
set -e

. ../common.sh

VERSION=14.4.2
prep https://downloads.sourceforge.net/project/sox/sox/${VERSION}/sox-${VERSION}.tar.gz sox-${VERSION}.tar.gz sox-${VERSION}

./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
