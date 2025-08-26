#!/bin/sh
set -e

. ../common.sh

VERSION=9.1.1686
prep https://github.com/vim/vim/archive/refs/tags/v${VERSION}.tar.gz v${VERSION}.tar.gz vim-${VERSION}

export vim_cv_toupper_broken=no
export vim_cv_terminfo=yes
export vim_cv_tgetent=yes
export vim_cv_getcwd_broken=no
export vim_cv_stat_ignores_slash=yes
export vim_cv_memmove_handles_overlap=yes
export CFLAGS=-I${SYSROOT}/usr/local/include/ncurses
rm -f src/auto/osdef.h
./configure --host=${ARCH}-pc-elf --with-tlib=tinfo --with-features=tiny
make || true
patch -p1 < ../osdef-remove.patch
make -j$(nproc)
make DESTDIR=$SYSROOT install
