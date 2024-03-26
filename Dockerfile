FROM docker.io/debian
RUN apt-get update && apt-get dist-upgrade -y
RUN apt-get install -y git curl build-essential nasm texinfo libgmp-dev libmpfr-dev libmpc-dev lzip e2tools grub2-common xorriso parted qemu-system-x86 sysvbanner
COPY . /usr/src/os/
WORKDIR /usr/src/os
RUN make kernel.bin initrd && make clean-cache
ENTRYPOINT qemu-system-i386 -s -m 128m -no-reboot -kernel kernel.bin -append "console=/dev/serial0" -initrd initrd -nographic
