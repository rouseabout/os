#!/bin/sh
set -e

. ../common.sh

VERSION=0.5.12
prep http://gondor.apana.org.au/~herbert/dash/files/dash-${VERSION}.tar.gz dash-${VERSION}.tar.gz dash-${VERSION}

./autogen.sh
./configure --host=i686-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
