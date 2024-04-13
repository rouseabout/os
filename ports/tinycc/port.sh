#!/bin/sh
set -e

. ../common.sh

test -d tinycc || git clone git://repo.or.cz/tinycc.git
cd tinycc
apply_patches

# first use host compiler to build and install libtcc1.a
mkdir -p host
cd host
../configure \
    --sysincludepaths=${SYSROOT}/usr/include:$PWD/../include \
    --libpaths=${SYSROOT}/usr/lib \
    --crtprefix=${SYSROOT}/usr/lib \
    --triplet=${ARCH}-pc-elf \
    --cpu=${ARCH} \
    --targetos=os \
    --config-bcheck=no \
    --config-backtrace=no \
    --config-predefs=no
make libtcc1.a -j$(nproc)
cp libtcc1.a ${SYSROOT}/usr/lib
cd ..

# then use toolchain to build and install tcc
mkdir -p target
cd target
../configure \
    --crtprefix=/usr/lib \
    --cross-prefix="${ARCH}-pc-elf-" \
    --cpu=${ARCH} \
    --triplet=${ARCH}-pc-elf \
    --cpu=${ARCH} \
    --targetos=os \
    --config-bcheck=no \
    --config-backtrace=no \
    --config-predefs=no
make tcc -j$(nproc)
make DESTDIR=$SYSROOT install
cd ..
