#!/bin/sh
set -e

. ../common.sh

VERSION=3.3
prep http://download.savannah.gnu.org/releases/dmidecode/dmidecode-${VERSION}.tar.xz dmidecode-${VERSION}.tar.xz dmidecode-${VERSION}

CC=i686-pc-elf-gcc make -j$(nproc)
make DESTDIR=$SYSROOT install
