#!/bin/sh
set -e

. ../common.sh

test -d tinycc || git clone https://github.com/TinyCC/tinycc
cd tinycc
apply_patches

./configure \
	--cross-prefix="i686-pc-elf-" \
	--enable-cross \
        --cpu=i686 \
        --triplet="i686-pc-elf" \
        --crtprefix=/usr/lib \
	--enable-static \
	--debug
make -j$(nproc) tcc CONFIG_pthread=no
make DESTDIR=$SYSROOT install
