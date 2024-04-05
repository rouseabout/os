#!/bin/sh
set -e

. ../common.sh

VERSION=5.1.16
prep https://ftp.gnu.org/gnu/bash/bash-${VERSION}.tar.gz bash-${VERSION}.tar.gz bash-${VERSION}

export bash_cv_getenv_redef=no
export bash_cv_getcwd_malloc=yes
export ac_cv_lib_dl_dlopen=no # disables building 'loadables'
export CFLAGS=-DNEED_EXTERN_PC
./configure --host=${ARCH}-pc-elf --without-bash-malloc
make -j$(nproc)
make DESTDIR=$SYSROOT install
