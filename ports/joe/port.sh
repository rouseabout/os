#!/bin/sh
set -e

. ../common.sh

VERSION=4.6
prep https://ixpeering.dl.sourceforge.net/project/joe-editor/JOE%20sources/joe-${VERSION}/joe-${VERSION}.tar.gz joe-${VERSION}.tar.gz joe-${VERSION}

./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
