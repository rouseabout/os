#!/bin/sh
set -e

. ../common.sh

VERSION=2.23
prep http://ftp.gnu.org/gnu/units/units-${VERSION}.tar.gz units-${VERSION}.tar.gz units-${VERSION}

./configure --host=i686-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
