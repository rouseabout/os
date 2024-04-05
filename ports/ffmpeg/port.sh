#!/bin/sh
set -e

. ../common.sh

VERSION=6.1.1
prep https://ffmpeg.org/releases/ffmpeg-${VERSION}.tar.xz ffmpeg-${VERSION}.tar.xz ffmpeg-${VERSION}

./configure --cross-prefix=${ARCH}-pc-elf- --arch=${ARCH} --target-os=none
make -j$(nproc)
make DESTDIR=$SYSROOT install
