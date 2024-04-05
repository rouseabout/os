#!/bin/sh
set -e

. ../common.sh

test -d tinycc || git clone https://github.com/TinyCC/tinycc
cd tinycc
apply_patches

./configure \
	--cross-prefix="${ARCH}-pc-elf-" \
	--enable-cross \
        --cpu=${ARCH} \
        --triplet="${ARCH}-pc-elf" \
        --crtprefix=/usr/lib \
	--enable-static \
	--debug
make -j$(nproc) tcc CONFIG_pthread=no
make DESTDIR=$SYSROOT install
