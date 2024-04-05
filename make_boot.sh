#!/bin/sh
set -e

stmp=$(mktemp)
ktmp=$(mktemp)
itmp=$(mktemp)
btmp=$(mktemp)

cleanup(){
    rm -f $ktmp $itmp $btmp
}

trap cleanup EXIT

dd if=kernel.linux16 of=$stmp bs=512 skip=1 conv=sync
sbytes=$(stat --format=%s $stmp)
ssects=$(expr $sbytes / 512)

dd if=kernel.linux32 of=$ktmp bs=512 conv=sync
kbytes=$(stat --format=%s $ktmp)
ksects=$(expr $kbytes / 512)

dd if=initrd of=$itmp bs=512 conv=sync
ibytes=$(stat --format=%s $itmp)
isects=$(expr $ibytes / 512)

nasm -f bin -o $btmp ./loader/loader.asm -DSETUP_SECTORS=$ssects -DKERNEL_SECTORS=$ksects -DINITRD_SECTORS=$isects

cat $btmp $stmp $ktmp $itmp > boot.bin
