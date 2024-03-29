#!/bin/sh
set -e

. ../common.sh

VERSION=3.20
test -e $CACHE/bwbasic-${VERSION}.zip || curl --remote-name --output-dir $CACHE --location https://downloads.sourceforge.net/project/bwbasic/bwbasic/version%20${VERSION}/bwbasic-${VERSION}.zip
mkdir -p bwbasic
cd bwbasic
unzip -o $CACHE/bwbasic-${VERSION}.zip

fromdos configure
export CC=i686-pc-elf-gcc
sh ./configure
make -j$(nproc)
mkdir -p $SYSROOT/usr/local/bin
make prefix=$SYSROOT/usr/local install
