#!/bin/sh
set -e

. ../common.sh

VERSION=1.14
prep http://ftp.gnu.org/gnu/moe/moe-${VERSION}.tar.lz moe-${VERSION}.tar.lz moe-${VERSION}

./configure CXX=i686-pc-elf-g++ CXXFLAGS=-I${SYSROOT}/usr/local/include/ncurses LIBS="-lncurses -ltinfo"
make -j$(nproc)
make DESTDIR=$SYSROOT install
