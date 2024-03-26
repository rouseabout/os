#!/bin/sh
set -e

. ../common.sh

VERSION=6.2
prep https://www.nano-editor.org/dist/v6/nano-${VERSION}.tar.xz nano-${VERSION}.tar.xz nano-${VERSION}

export CFLAGS=-I${SYSROOT}/usr/local/include/ncurses
./configure --host=i686-pc-elf
sed -i 's/ncursesw/ncurses/g' src/Makefile
sed -i 's/mbtowc_with_lock/mbtowc/g' lib/mbrtowc-impl.h
make -j$(nproc)
make DESTDIR=$SYSROOT install
