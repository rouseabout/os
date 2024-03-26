#!/bin/sh
set -e

. ../common.sh

VERSION=9.0
prep https://ftp.gnu.org/gnu/coreutils/coreutils-${VERSION}.tar.xz coreutils-${VERSION}.tar.xz coreutils-${VERSION}

./configure --host=i686-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
