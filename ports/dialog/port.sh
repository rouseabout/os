#!/bin/sh
set -e

. ../common.sh

VERSION=1.3-20240307
prep https://invisible-mirror.net/archives/dialog/dialog-${VERSION}.tgz dialog-${VERSION}.tgz dialog-${VERSION}

./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
