include .config
HOSTCC=cc
HOSTCCFLAGS=
TOOLCHAIN=$(PWD)/toolchain-$(ARCH)-pc-elf
CC=$(TOOLCHAIN)/bin/$(ARCH)-pc-elf-gcc
CXX=$(TOOLCHAIN)/bin/$(ARCH)-pc-elf-g++
AR=$(TOOLCHAIN)/bin/$(ARCH)-pc-elf-ar
STRIP=$(TOOLCHAIN)/bin/$(ARCH)-pc-elf-strip
READELF=$(TOOLCHAIN)/bin/$(ARCH)-pc-elf-readelf
OBJDUMP=$(TOOLCHAIN)/bin/$(ARCH)-pc-elf-objdump
CFLAGS=-O3 -Wall -pedantic -Wshadow -Wextra -Werror=format-security -Werror=implicit-function-declaration -Werror=missing-prototypes -Werror=pointer-arith -Werror=return-type -Werror=vla -Werror=logical-op -Wno-unused-parameter -Wno-sign-compare -g -DARCH_$(ARCH)
KERNELCFLAGS=-ffreestanding -Ilibc -Ilibdl -Ilibm
ifeq ($(ARCH),x86_64)
KERNELCFLAGS+=-mcmodel=large
endif
CXXFLAGS=-O3
LD=$(CC)
LDFLAGS=-ffreestanding -nostdlib -g -Lsysroot/usr/lib
QEMU=qemu-system-$(QEMUARCH)
QEMUFLAGS+=-s -m 128m -no-reboot
#QEMUFLAGS+=-netdev socket,id=net0,mcast=230.0.0.1:1234 -device ne2k_pci,netdev=net0,mac=00:11:22:33:44:55
QEMUFLAGS+=-debugcon stdio
#QEMUFLAGS+=-S  #(wait for debugger connection)
#QEMUFLAGS+=-enable-kvm

ifeq ($(ARCH),x86_64)
all: qemu-linux
else
all: qemu
endif

qemu: kernel.bin initrd
	$(QEMU) $(QEMUFLAGS) -kernel $< -append "" -initrd initrd

qemu-serial: kernel.bin initrd
	$(QEMU) $(subst -debugcon stdio,,$(QEMUFLAGS)) -kernel $< -append "console=/dev/serial0" -initrd initrd -nographic

qemu-hd-initrd: kernel.bin initrd
	$(QEMU) $(QEMUFLAGS) -kernel $< -append "root=/dev/hda" -drive if=ide,file=initrd,format=raw,index=0,media=disk

qemu-hd: disk_image
	$(QEMU) $(QEMUFLAGS) -drive if=ide,file=disk_image,format=raw,index=0,media=disk

qemu-hd-serial: disk_image
	$(QEMU) $(subst -debugcon stdio,,$(QEMUFLAGS)) -drive if=ide,file=disk_image,format=raw,index=0,media=disk -nographic

qemu-efi: efi/EFI/BOOT/BOOT$(EFIARCH).EFI efi/kernel efi/initrd
	$(QEMU) $(QEMUFLAGS) -bios /usr/share/ovmf/OVMF.fd -drive file=fat:rw:efi,format=raw

efi-cdrom.iso: efi/EFI/BOOT/BOOT$(EFIARCH).EFI efi/kernel efi/initrd
	dd if=/dev/zero of=$@ bs=1M count=6
	mformat -i $@ ::
	mmd -i $@ EFI
	mmd -i $@ EFI/BOOT
	mcopy -i $@ efi/EFI/BOOT/BOOT$(EFIARCH).EFI ::EFI/BOOT
	mcopy -i $@ efi/kernel efi/initrd ::

qemu-efi-cdrom: efi-cdrom.iso
	$(QEMU) $(QEMUFLAGS) -bios /usr/share/ovmf/OVMF.fd -cdrom $<

efi/EFI/BOOT/BOOT$(EFIARCH).EFI: loader/efiloader.c
	$(ARCH)-w64-mingw32-gcc -o loader/efiloader.o -c loader/efiloader.c -Wall -ffreestanding -I/usr/include/efi/
	$(ARCH)-w64-mingw32-gcc -o $@ loader/efiloader.o -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e $(EFIMAIN)

disk_image: make_disk_image.sh hd/boot/grub/grub.cfg hd/boot/kernel.bin hd/boot/initrd sysroot
	sudo --help >/dev/null 2>&1 && sudo ./make_disk_image.sh || ./make_disk_image.sh

%.vdi: %
	qemu-img convert $< -O vdi $@

qemu-cdrom: cdrom.iso
	$(QEMU) $(QEMUFLAGS) -cdrom $<

qemu-usb-cdrom: cdrom.iso
	$(QEMU) $(QEMUFLAGS) -drive id=pendrive,file=cdrom.iso,format=raw,if=none -device usb-storage,drive=pendrive -usb

bochs-cdrom: cdrom.iso
	bochs -f bochsrc-cdrom -q

bochs-hd: disk_image
	bochs -f bochsrc-hd -q

temu-hd: disk_image
	temu -m 128 temu-hd.cfg

cdrom.iso: iso/boot/grub/grub.cfg iso/boot/kernel.bin iso/boot/initrd
	grub-mkrescue -o $@ --compress=xz --fonts="" --locales="" iso

TFTP_FILES=$(addprefix tftp/,pxelinux.0 ldlinux.c32 libcom32.c32 libutil.c32 menu.c32 mboot.c32 kernel.bin initrd)
qemu-pxe: $(TFTP_FILES)
	$(QEMU) $(QEMUFLAGS) \
		-netdev user,id=net0,net=192.168.88.0/24,tftp=$(PWD)/tftp/,bootfile=/pxelinux.0 \
		-device virtio-net-pci,netdev=net0 -boot n

qemu-pxe-serial: $(TFTP_FILES)
	$(QEMU) $(subst -debugcon stdio,,$(QEMUFLAGS)) \
		-netdev user,id=net0,net=192.168.88.0/24,tftp=$(PWD)/tftp/,bootfile=/pxelinux.0 \
		-device virtio-net-pci,netdev=net0 -boot n \
		-nographic

hd/boot/%: %
	cp $< $@

iso/boot/%: %
	cp $< $@

tftp/%: %
	cp $< $@

efi/%: %
	cp $< $@

temu-linux: kernel.linux initrd
	temu temu-linux.cfg

qemu-linux: kernel.linux initrd
	$(QEMU) $(QEMUFLAGS) -kernel kernel.linux -initrd initrd

qemu-linux-serial: kernel.linux initrd
	$(QEMU) $(subst -debugcon stdio,,$(QEMUFLAGS)) -kernel kernel.linux -initrd initrd -append "console=/dev/serial0" -nographic

%.o: %.asm
	nasm -f $(NASMFORMAT) -DARCH_$(ARCH) -o $@ $<

#libc modules common to kernel and libc.a
LIBC_COMMON_OBJS=$(addprefix libc/,bsd_string.o ctype.o heap.o langinfo.o libgen.o signal.o stdio.o stdlib.o string.o strings.o time.o) $(addprefix libm/,math.o)

#libc modules only used by libc.a
LIBC_ONLY_OBJS=$(addprefix libc/,arpa_inet.o dirent.o errno.o fcntl.o fnmatch.o getopt.o grp.o inttypes.o locale.o mntent.o netdb.o net_if.o netinet_in.o poll.o pthread.o pwd.o regex.o sched.o semaphore.o $(ARCH)/setjmp.o signal2.o stdio2.o stdlib2.o string2.o sys_ioctl.o sys_mman.o sys_resource.o sys_select.o sys_socket.o sys_stat.o sys_statvfs.o sys_time.o sys_times.o sys_uio.o sys_utsname.o sys_wait.o syslog.o termios.o time2.o unistd.o utime.o wchar.o wctype.o crt0.o)

KERNEL_OBJS=$(addprefix kernel/,$(ARCH)/start2.o $(ARCH)/common.o acpi.o ata.o dev.o ext2.o fb.o kb.o loop.o main.o mem.o ne2k.o pci.o pipe.o power.o proc.o serial.o textmode.o tty.o vfs.o) $(LIBC_COMMON_OBJS)
kernel.bin: kernel/linker.ld kernel/multiboot.o $(KERNEL_OBJS) .toolchain-$(ARCH)-stage1
	$(LD) $(LDFLAGS) -o $@ -Wl,--defsym,ARCH_$(ARCH)=1 -T kernel/linker.ld -Wl,-Map,kernel.map kernel/multiboot.o $(KERNEL_OBJS) -lgcc
	$(STRIP) --strip-all $@

kernel.linux: kernel.linux16 kernel.linux32
	cat $^ > $@

kernel.linux16: kernel/linux16.asm
	nasm -f bin -o $@ $<

kernel.linux32: kernel/linker.ld kernel/linux32.o $(KERNEL_OBJS) .toolchain-$(ARCH)-stage1
	$(LD) $(LDFLAGS) -o $@ -Wl,--oformat=binary -Wl,--defsym,ARCH_$(ARCH)=1 -T kernel/linker.ld -Wl,-Map,kernel.map kernel/linux32.o $(KERNEL_OBJS) -lgcc

efi/kernel: kernel/linker.ld kernel/$(ARCH)/efi.o $(KERNEL_OBJS) .toolchain-$(ARCH)-stage1
	$(LD) $(LDFLAGS) -o $@ -Wl,--oformat=binary -Wl,--defsym,ARCH_$(ARCH)=1 -T kernel/linker.ld -Wl,-Map,kernel.map kernel/$(ARCH)/efi.o $(KERNEL_OBJS) -lgcc

libc.a: $(LIBC_COMMON_OBJS) $(LIBC_ONLY_OBJS) .toolchain-$(ARCH)-stage1
	$(AR) rcs $@ $^

libdl.a: libdl/dlfcn.o .toolchain-$(ARCH)-stage1
	$(AR) rcs $@ $^

libm.a: libm/math.o .toolchain-$(ARCH)-stage1
	$(AR) rcs $@ $^

libg.a: libg/dummy.o .toolchain-$(ARCH)-stage1
	$(AR) rcs $@ $^

programs/box.o: $(wildcard programs/box_*.c)

programs/%.o: programs/%.c .toolchain-$(ARCH)-stage2 .sysroot
	$(CC) -o $@ -c $< $(CFLAGS)

programs/%++.o: programs/%++.cc .toolchain-$(ARCH)-stage2 .sysroot
	$(CXX) -o $@ -c $< $(CXXFLAGS)

crash: programs/crash.o .toolchain-$(ARCH)-stage2 .sysroot
	$(LD) $(LDFLAGS) -o $@ $<

hello: programs/hello.o .toolchain-$(ARCH)-stage2 .sysroot
	$(LD) $(LDFLAGS) -o $@ $<

%++: programs/%++.o .toolchain-$(ARCH)-stage2 .sysroot
	$(CXX) -o $@ $<
	$(STRIP) --strip-all $@

%: programs/%.o libc.a libm.a .toolchain-$(ARCH)-stage2 .sysroot
	$(CC) -o $@ $<
	$(STRIP) --strip-all $@

%.o: %.cc .toolchain-$(ARCH)-stage1
	$(CXX) -o $@ -c $< $(CXXFLAGS)

%.o: %.c .toolchain-$(ARCH)-stage1
	$(CC) -o $@ -c $< $(CFLAGS) $(KERNELCFLAGS)

d-%: %
	($(READELF) -h $<; $(OBJDUMP) -h $<;  $(OBJDUMP) -d -l $<) | less

gdb: kernel.bin
	gdb $< -ex 'target remote localhost:1234'

clean:
	rm -f kernel.bin kernel.linux* kernel.map $(KERNEL_OBJS) kernel/multiboot.o kernel/linux32.o cdrom.iso iso/boot/kernel.bin initrd iso/boot/initrd disk_image disk_image.vdi hd/boot/kernel.bin hd/boot/initrd $(TFTP_FILES) programs/*.o libc/*.o libc/$(ARCH)/*.o libdl/*.o libg/*.o libm/*.o $(shell echo $(PROGRAMS)) libc.a libdl.a libg.a libm.a $(TEST_BIN) $(DUMPELF_BIN) $(DUMPEXT2_BIN) $(EXT2TEST_BIN) efimain.o efi/kernel efi/initrd efi/EFI/BOOT/BOOT$(EFIARCH).EFI .sysroot
	rm -rf sysroot

initrd: $(shell echo $(PROGRAMS)) $(wildcard scripts/*) README.md programs/hello.asm
	rm -f $@
	/sbin/mkfs.ext2 -r 1 -b 4096 $@ $(INITRDSIZE)
	e2cp -p $(shell echo $(PROGRAMS)) scripts/* $@:bin/
	for x in $(shell echo $(BOXPROGRAMS)); do e2ln initrd:/bin/box bin/$$x; done
	e2mkdir $@:/tmp
	e2cp README.md programs/hello.asm $@:

HEADERS=\
	alloca.h \
	assert.h \
	ctype.h \
	dirent.h \
	elf.h \
	errno.h \
	features.h \
	fcntl.h \
	fnmatch.h \
	getopt.h \
	grp.h \
	heap.h \
	inttypes.h \
	langinfo.h \
	libgen.h \
	limits.h \
	locale.h \
	memory.h \
	netdb.h \
	nl_types.h \
	poll.h \
	pthread.h \
	pwd.h \
	regex.h \
	sched.h \
	semaphore.h \
	setjmp.h \
	signal.h \
	stdint.h \
	stdio.h \
	stdlib.h \
	string.h \
	strings.h \
	syslog.h \
	termios.h \
	time.h \
	unistd.h \
	utime.h \
	utmp.h \
	wchar.h \
	wctype.h \
	\
	arpa/inet.h \
	\
	bsd/string.h \
	\
	linux/fb.h \
	linux/kd.h \
	linux/keyboard.h \
	\
	net/if.h \
	\
	netinet/in.h \
	netinet/tcp.h \
	\
	os/syscall.h \
	\
	sys/fcntl.h \
	sys/ioctl.h \
	sys/mman.h \
	sys/param.h \
	sys/resource.h \
	sys/select.h \
	sys/socket.h \
	sys/stat.h \
	sys/statvfs.h \
	sys/time.h \
	sys/times.h \
	sys/types.h \
	sys/ucontext.h \
	sys/uio.h \
	sys/un.h \
	sys/utsname.h \
	sys/wait.h \
	\
	dlfcn.h \
	\
	math.h

sysroot/usr/include/%: libc/%
	mkdir -p $(shell dirname $@)
	cp $< $@

sysroot/usr/include/%: libdl/%
	mkdir -p $(shell dirname $@)
	cp $< $@

sysroot/usr/include/%: libm/%
	mkdir -p $(shell dirname $@)
	cp $< $@

sysroot/usr/lib/%: %
	mkdir -p $(shell dirname $@)
	cp $< $@

sysroot/usr/lib/%: libc/% # needed for crt0.o
	mkdir -p $(shell dirname $@)
	cp $< $@

.sysroot: .toolchain-$(ARCH)-stage1 $(addprefix sysroot/usr/include/,$(HEADERS)) $(addprefix sysroot/usr/lib/,crt0.o libc.a libdl.a libg.a libm.a)
	touch $@

CACHE=.cache
BINUTILSVER=2.43.1
GCCVER=14.2.0
SYSLINUXVER=6.03
$(CACHE)/binutils-$(BINUTILSVER).tar.xz:
	curl https://ftp.gnu.org/gnu/binutils/binutils-$(BINUTILSVER).tar.xz -o $@
$(CACHE)/gcc-$(GCCVER).tar.xz:
	curl https://ftp.gnu.org/gnu/gcc/gcc-$(GCCVER)/gcc-$(GCCVER).tar.xz -o $@
$(CACHE)/syslinux-$(SYSLINUXVER).zip:
	curl https://www.zytor.com/pub/syslinux/syslinux-$(SYSLINUXVER).zip -o $@
clean-cache:
	rm -rf $(CACHE)

TARGET=$(ARCH)-pc-elf
SYSROOT=$(PWD)/sysroot
MAKEFLAGS=-j$(shell nproc)

.toolchain-$(ARCH)-binutils: $(CACHE)/binutils-$(BINUTILSVER).tar.xz
	tar xf $(CACHE)/binutils-$(BINUTILSVER).tar.xz
	mkdir -p build-binutils && \
		cd build-binutils && \
		../binutils-$(BINUTILSVER)/configure --target=$(TARGET) --prefix=$(TOOLCHAIN) --with-sysroot --disable-nls --disable-werror && \
		$(MAKE) $(MAKEFLAGS) && \
		$(MAKE) install
	rm -rf build-binutils binutils-$(BINUTILSVER)
	touch $@

.toolchain-$(ARCH)-stage1: | .toolchain-$(ARCH)-binutils $(CACHE)/gcc-$(GCCVER).tar.xz
	tar xf $(CACHE)/gcc-$(GCCVER).tar.xz
	cd gcc-$(GCCVER) && \
		patch -p1 < ../patches/0001-libstdc-support-pc-elf.patch && \
		patch -p1 < ../patches/0004-disable-fixincludes-for-elf.patch
	mkdir -p build-gcc-stage1 && \
		cd build-gcc-stage1 && \
		../gcc-$(GCCVER)/configure --target=$(TARGET) --prefix=$(TOOLCHAIN) --disable-nls --enable-languages=c --without-headers && \
		$(MAKE) $(MAKEFLAGS) all-gcc && \
		$(MAKE) $(MAKEFLAGS) all-target-libgcc && \
		$(MAKE) install-gcc && \
		$(MAKE) install-target-libgcc
	rm -rf build-gcc-stage1 gcc-$(GCCVER)
	touch $@

.toolchain-$(ARCH)-stage2: | .toolchain-$(ARCH)-binutils .sysroot $(CACHE)/gcc-$(GCCVER).tar.xz
	tar xf $(CACHE)/gcc-$(GCCVER).tar.xz
	cd gcc-$(GCCVER) && \
		patch -p1 < ../patches/0001-libstdc-support-pc-elf.patch && \
		patch -p1 < ../patches/0004-disable-fixincludes-for-elf.patch
	mkdir -p build-gcc-stage2 && \
		cd build-gcc-stage2 && \
		../gcc-$(GCCVER)/configure --target=$(TARGET) --prefix=$(TOOLCHAIN) --disable-nls --enable-languages=c,c++ --with-sysroot=$(SYSROOT) --without-headers && \
		$(MAKE) $(MAKEFLAGS) all-gcc && \
		$(MAKE) $(MAKEFLAGS) all-target-libgcc && \
		$(MAKE) $(MAKEFLAGS) all-target-libstdc++-v3 && \
		$(MAKE) install-gcc && \
		$(MAKE) install-target-libgcc && \
		$(MAKE) install-target-libstdc++-v3
	rm -rf build-gcc-stage2 gcc-$(GCCVER)
	touch .toolchain-$(ARCH)-stage1 $@

clean-toolchain:
	rm -rf $(TOOLCHAIN) .toolchain-$(ARCH)-binutils .toolchain-$(ARCH)-stage1 .toolchain-$(ARCH)-stage2

define TFTP
tftp/$(2): $(CACHE)/syslinux-$(SYSLINUXVER).zip
	unzip -p $$< bios/$(1)/$(2) > $$@
endef
$(eval $(call TFTP,core,pxelinux.0))
$(eval $(call TFTP,com32/elflink/ldlinux,ldlinux.c32))
$(eval $(call TFTP,com32/lib,libcom32.c32))
$(eval $(call TFTP,com32/libutil,libutil.c32))
$(eval $(call TFTP,com32/menu,menu.c32))
$(eval $(call TFTP,com32/mboot,mboot.c32))

tarball:
	basename=$(shell basename $(shell pwd)) && cd .. && tar -cf - --exclude .git $$basename | gzip -9 - > os.tar.gz

EXT2TEST_BIN=ext2test
test:: $(EXT2TEST_BIN) initrd
	./$^
$(EXT2TEST_BIN): kernel/dev.c kernel/ext2.c kernel/pipe.c kernel/vfs.c contrib/ext2test.c
	$(HOSTCC) $(HOSTCCFLAGS) -DTEST=1 -Ikernel -o $@ $^ -g -lbsd

DUMPELF_BIN=dumpelf
#test:: $(DUMPELF_BIN) init
#	./$^
$(DUMPELF_BIN): contrib/dumpelf.c
	$(HOSTCC) $(HOSTCCFLAGS) -Ilibc -DARCH_$(ARCH) -o $@ $^ -g

DUMPEXT2_BIN=dumpext2
#test:: $(DUMPEXT2_BIN) initrd
#	./$^
$(DUMPEXT2_BIN): contrib/dumpext2.c
	$(HOSTCC) $(HOSTCCFLAGS) -Ikernel -o $@ $^ -g

DUMPGPT_BIN=dumpgpt
#test:: $(DUMPGPT_BIN) disk_image
#	./$^
$(DUMPGPT_BIN): contrib/dumpgpt.c
	$(HOSTCC) $(HOSTCCFLAGS) -o $@ $^ -g

HEAPTEST_BIN=heaptest
test:: $(HEAPTEST_BIN)
	./$^
$(HEAPTEST_BIN): libc/heap.c libc/bsd_string.c
	$(HOSTCC) $(HOSTCCFLAGS) -DTEST=1 -o $@ $^ -g

test:: kernel.linux initrd
	$(QEMU) $(subst -debugcon stdio,,$(QEMUFLAGS)) -kernel kernel.linux -initrd initrd -append "init=/bin/testcases console=/dev/serial0" -nographic

clean-tests:
	rf -f $(EXT2TEST_BIN) $(DUMPELF_BIN) $(DUMPEXT2_BIN) $(DUMPGPT_BIN) $(HEAPTEST_BIN)

qemu-boot: boot.bin
	$(QEMU) $(QEMUFLAGS) -drive if=ide,file=$^,format=raw,index=0,media=disk

qemu-usb-boot: boot.bin
	$(QEMU) $(QEMUFLAGS) -drive id=pendrive,file=$^,format=raw,if=none -device usb-storage,drive=pendrive -usb

bochs-boot: boot.bin
	bochs -f bochsrc-boot -q

temu-boot: boot.bin
	temu -m 128 temu-boot.cfg

boot.bin: loader/loader.asm kernel.linux16 kernel.linux32 initrd
	./make_boot.sh
