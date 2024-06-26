#!/bin/sh
set -e

. ../common.sh

prep https://invisible-island.net/datafiles/release/vttest.tar.gz vttest.tar.gz "vttest-*"

./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
