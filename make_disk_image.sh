#!/bin/sh
set -e
. ./.config

filename=disk_image
size=1024 #megabytes

dd if=/dev/zero of=$filename bs=1M count=$size status=none

chown "$SUDO_UID":"$SUDO_GID" $filename

dev=$(losetup --find --partscan --show $filename)

cleanup(){
    if test -d mnt; then
        umount mnt
        rmdir mnt
    fi
    losetup -d ${dev}
}

trap cleanup EXIT

parted -s ${dev} \
	mklabel msdos \
	mkpart primary ext2 1MiB 100% \
	set 1 boot on

dd if=/dev/zero of=${dev}p1 bs=1M count=$(expr $size - 1) status=none

mke2fs -q ${dev}p1 -b 4096

mkdir -p mnt
mount ${dev}p1 mnt/

cp -r hd/* mnt/

mkdir mnt/bin
PROGRAMS="cat chat clear cmp crash date draw echo env false flash forkbomb hello hello++ hexdump hostname init kill ln ls mkdir more mount mv pwd reset rm rmdir sh sleep testcases tolower touch tree true truncate uname unixping unixserver wc"
cp $PROGRAMS mnt/bin
cp README.md mnt/
mkdir mnt/tmp
cp -R sysroot/* mnt/
find mnt -type f -executable -exec toolchain-${ARCH}-pc-elf/bin/${ARCH}-pc-elf-strip {} \;

grub-install --boot-directory=mnt/boot --target=i386-pc --modules="ext2 part_msdos" ${dev}
