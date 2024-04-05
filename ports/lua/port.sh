#!/bin/sh
set -e

. ../common.sh

VERSION=5.4.6
prep https://www.lua.org/ftp/lua-${VERSION}.tar.gz lua-${VERSION}.tar.gz lua-${VERSION}

make INSTALL_TOP=$SYSROOT/usr/local CC=${ARCH}-pc-elf-gcc RANLIB="${ARCH}-pc-elf-ranlib" AR="${ARCH}-pc-elf-ar rcu" posix install
