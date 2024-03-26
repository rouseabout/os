#!/bin/sh
set -e

. ../common.sh

VERSION=7.16
prep http://www.ibiblio.org/pub/Linux/apps/financial/spreadsheet/sc-${VERSION}.tar.gz sc-${VERSION}.tar.gz sc-${VERSION}

make CC=i686-pc-elf-gcc CFLAGS="-DSYSV3 -I${SYSROOT}/usr/local/include/ncurses" LIB="-lm -lncurses -ltinfo" -j$(nproc) 
make prefix=$SYSROOT/usr/local install
