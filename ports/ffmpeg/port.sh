#!/bin/sh
set -e

. ../common.sh

VERSION=6.1.1
prep https://ffmpeg.org/releases/ffmpeg-${VERSION}.tar.xz ffmpeg-${VERSION}.tar.xz ffmpeg-${VERSION}

./configure --cross-prefix=i686-pc-elf- --arch=i686 --target-os=none
make -j$(nproc)
make DESTDIR=$SYSROOT install
