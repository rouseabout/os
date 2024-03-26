#!/bin/sh
set -e

. ../common.sh

VERSION=2.2.12
prep http://ftp.mutt.org/pub/mutt/mutt-${VERSION}.tar.gz mutt-${VERSION}.tar.gz mutt-${VERSION}

export CFLAGS="-I${SYSROOT}/usr/local/include/ncurses -Dcaddr_t=void*"
export LIBS=-ltinfo
./configure --host=i686-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
