#!/bin/sh
set -e

. ../common.sh

VERSION=2.7.17
prep https://www.python.org/ftp/python/${VERSION}/Python-${VERSION}.tar.xz Python-${VERSION}.tar.xz Python-${VERSION}

export ac_cv_file__dev_ptmx=no
export ac_cv_file__dev_ptc=no
./configure --host=i686-pc-elf --build=x86_64-pc-linux-gnu --disable-ipv6 --disable-shared --without-threads
make -j$(nproc)
make DESTDIR=$SYSROOT install
