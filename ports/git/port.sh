#!/bin/sh
set -e

. ../common.sh

VERSION=2.44.0
prep https://mirrors.edge.kernel.org/pub/software/scm/git/git-${VERSION}.tar.xz git-${VERSION}.tar.xz git-${VERSION}

export ac_cv_iconv_omits_bom=no
export ac_cv_fread_reads_directories=no
export ac_cv_snprintf_returns_bogus=no
./configure --host=${ARCH}-pc-elf
make -j$(nproc)
make DESTDIR=$SYSROOT install
