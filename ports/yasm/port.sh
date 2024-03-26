#!/bin/sh
set -e

. ../common.sh

VERSION=1.3.0
prep http://www.tortall.net/projects/yasm/releases/yasm-${VERSION}.tar.gz yasm-${VERSION}.tar.gz yasm-${VERSION}

./configure --host=i686-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
