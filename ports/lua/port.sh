#!/bin/sh
set -e

. ../common.sh

VERSION=5.4.6
prep https://www.lua.org/ftp/lua-${VERSION}.tar.gz lua-${VERSION}.tar.gz lua-${VERSION}

make INSTALL_TOP=$SYSROOT/usr/local CC=i686-pc-elf-gcc RANLIB="i686-pc-elf-ranlib" AR="i686-pc-elf-ar rcu" posix install
