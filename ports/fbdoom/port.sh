#!/bin/sh
set -e

. ../common.sh

test -d fbdoom || git clone https://github.com/maximevince/fbDOOM fbdoom
cd fbdoom
apply_patches

cd fbdoom
make CROSS_COMPILE=${ARCH}-pc-elf- NOSDL=1 -j$(nproc)
mkdir -p $SYSROOT/usr/local/bin
cp fbdoom $SYSROOT/usr/local/bin
