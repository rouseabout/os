#!/bin/sh
set -e

. ../common.sh

VERSION=2.25
prep http://links.twibright.com/download/links-${VERSION}.tar.gz links-${VERSION}.tar.gz links-${VERSION}

export CC=i686-pc-elf-gcc
./configure --host=i686-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
