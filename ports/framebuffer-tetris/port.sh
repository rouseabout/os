#!/bin/sh
set -e

. ../common.sh

test -d framebuffer-tetris || git clone https://github.com/mzorro/framebuffer-tetris
cd framebuffer-tetris
apply_patches

make CC=${ARCH}-pc-elf-gcc -j$(nproc)
mkdir -p $SYSROOT/usr/local/bin
cp main $SYSROOT/usr/local/bin
