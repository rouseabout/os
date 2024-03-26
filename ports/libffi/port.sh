#!/bin/sh
set -e

. ../common.sh

VERSION=3.4.5
prep https://github.com/libffi/libffi/releases/download/v${VERSION}/libffi-${VERSION}.tar.gz libffi-${VERSION}.tar.gz libffi-${VERSION}

./configure --host=i686-pc-elf --target=i686-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
