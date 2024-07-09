#!/bin/sh
set -e

. ../common.sh

VERSION=7.16
prep http://www.ibiblio.org/pub/Linux/apps/financial/spreadsheet/sc-${VERSION}.tar.gz sc-${VERSION}.tar.gz sc-${VERSION}

make CC=${ARCH}-pc-elf-gcc CFLAGS="-Wno-implicit-function-declaration -Wno-return-mismatch -Wno-implicit-int -DSYSV3 -I${SYSROOT}/usr/local/include/ncurses" LIB="-lm -lncurses -ltinfo" -j$(nproc)
make prefix=$SYSROOT/usr/local install
