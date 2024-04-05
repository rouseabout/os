#!/bin/sh
set -e

. ../common.sh

test -d hexedit || git clone https://github.com/pixel/hexedit
cd hexedit
apply_patches

./autogen.sh
export CFLAGS=-I${SYSROOT}/usr/local/include/ncurses
export LDFLAGS=-L${SYSROOT}/usr/local/lib
./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
