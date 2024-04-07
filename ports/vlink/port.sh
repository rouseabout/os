#!/bin/sh
set -e

. ../common.sh

VERSION=0_17a
prep http://phoenix.owl.de/tags/vlink${VERSION}.tar.gz vlink${VERSION}.tar.gz vlink

make CC=${ARCH}-pc-elf-gcc -j$(nproc)
mkdir -p $SYSROOT/usr/local/bin
cp vlink $SYSROOT/usr/local/bin
