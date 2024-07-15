# OS

*os* is a toy Unix-like operating system for x86 computers.

Features:

* Pre-emptive multi-tasking/threading kernel.
* User and kernel process isolation; memory management.
* VT100 terminal emulator. Supports serial, text console and framebuffer graphics mode.
* Virtual file system with read/write ext2 file system.
* Minimal libc and libm implementation.
* Unix sockets.
* Includes simple user space programs such as sh, cat, echo and more.
* Capable of running various third-party programs including bash, gcc, git, python, vim, and fbdoom.

It is not robust, secure, or efficient, and should be considered highly experimental.

Live demonstration: <https://pross.sdf.org/sandpit/os/>

*os* has been tested on real x86 hardware (Intel Haswell, Intel Kaby Lake).
It has also been tested with various x86 emulators including Bochs (<https://bochs.sourceforge.io/>), PCem (<https://pcem-emulator.co.uk/>), QEMU (<https://www.qemu.org/>), TinyEMU (<https://bellard.org/tinyemu/>), v86 (<https://github.com/copy/v86>) and VirtualBox (<https://www.virtualbox.org/>).


## Quick start

To build and boot *os* using QEMU, first install the prerequisites. The following command works on Debian/Ubuntu.

```
apt-get install -y git curl build-essential nasm texinfo libgmp-dev libmpfr-dev libmpc-dev lzip e2tools grub2-common xorriso parted qemu-system-x86 tofrodos sysvbanner

```

Then type `make qemu`. This will download and build the gcc-based toolchain, build the `kernel.bin` and file system, and boot into QEMU.

Once booted, *os* runs the `init` process, which then executes `sh` in a forever loop.
There is no user login.
See below for a list of included programs.


## 64-bit operating system

By default *os* is 32-bit.
To build a 64-bit *os*, edit `.config` and set `ARCH=x86_64` etc., then perform `make clean` and `make`.
NASM >= 2.16 is required.


## Alternative boot methods

*os* can be booted many ways.
First it is important to understand the high-level design of the operating system.
*os* consists of a kernel file and sysroot directory. The `kernel.bin` file is compatible with the multiboot specification. The `kernel.linux` file is compatbile with the Linux boot protocol. The sysroot directory is copied to an ext2 file system, either intended to function as a ram disk (`initrd`) or a hard disk (`disk_image`).
When using multiboot with an initrd module, both kernel and initrd files must fit within the first 8 MiB of memory.


### ISO image boot

`make cdrom.iso` will produce bootable ISO image, where the GNU GRUB boot loader boots `kernel.bin` and `initrd`.
The ISO image can be copied to optical media or USB, and booted on real x86 hardware.
The ISO image can also be booted in QEMU using `make qemu-cdrom` or in Bochs using `make bochs-cdrom`.


### initrd as hard disk boot

When using the `make qemu` or `make qemu-cdrom` methods, initrd is copied into memory and served as a ramdisk.
Any changes to the root file system at boot time are lost on reboot.
Alternatively, initrd can be stored on a hard disk and content preserved across boots.
In this configuration, there are no partitions!
This involves setting the kernel command line string to include parameter to include `root=<ROOT-DEVICE>`, for example `root=/dev/hda`.

To boot in QEMU use `make qemu-hd-initrd`


### hard disk boot

`make disk_image` will produce a bootable hard disk image, with partition table and GNU GRUB boot loader.
At boot time, the boot loader will load kernel.bin, and then *os* will read files from the hard disk.

Please note that root/sudo access is required to make `disk_image`, because the `make_disk_image.sh` script uses a loopback device to mount the disk image.

To boot in QEMU use `make qemu-hd`. To boot in Bochs use `make bochs-hd`. To boot in TinyEMU use `make temu-hd`.


### Linux boot protocol

`kernel.linux` has been tested with GRUB, LILO, LINLD, LOADLIN and SYSLINUX.

To boot in QEMU use use `make qemu-linux`. To boot in TinyEMU use `make temu-linux`.


### os boot loader

No operating system is complete without its own boot loader!
`make boot.bin` will produce a custom boot image that can be written to a hard disk or USB drive.
The image contains no files, just the boot loader, kernel and initrd stored in contiguous sectors.

To boot in QEMU use use `make qemu-boot` or `make qemu-usb-boot`.To boot into Bochs use `make bochs-boot`. To boot in TinyEMU use `make temu-boot`.

## Installing third-party software

`disk_image` is populated from the sysroot directory.
To install third-party software, one needs to only copy files into sysroot and rebuild `disk_image`.

Scripts for downloading, building and installing third-party software can be found in the ports directory.
For complete list of ports see [ports/STATUS.md](ports/STATUS.md).

An example of building and running the fbdoom port is given below.

```
(cd ports/fbdoom && ./port)
(cd sysroot && curl --remote-name https://distro.ibiblio.org/slitaz/sources/packages/d/doom1.wad)
rm -f disk_image
make qemu-hd
```

Then type `fbdoom -iwad doom1.wad` into the *os* shell.


## Docker/Podman image

A Docker/Podman compatible configuration file is provided that builds *os* and boots into the serial console. To build and run, use these commands:

```
docker build -t os -f Dockerfile .
docker run -it os
```


## Experimental features

Below is a list of features deemed experimental and disabled by default.


### Kernel debugging

Kernel debug printf (kprintf) and syslog statements are littered throughout the code base.
To view these messages append `-debugcon stdio` to QEMUFLAGS in the Makefile then boot into *os*.
Pressing the PAUSE keyboard button prints a list of processes and free memory.

To attach gdb to the operating system append `-S` to QEMUFLAGS in the Makefile and then boot into *os*.
Then run `gdb kernel.bin`, and type `target remote localhost:1234` at the gdb prompt.


### Networking

*os* supports the NE2000 network device.
There is no Internet Protocol (IP) stack, therefore *os* is only capable of sending and receiving raw Ethernet frames.
To enable networking append `-netdev socket,id=net0,mcast=230.0.0.1:1234 -device ne2k_pci,netdev=net0,mac=00:11:22:33:44:55` options to QEMUFLAGS in the Makefile.
For an interactive chat session between computers, boot into *os*, type `chat /dev/net`, and on the host computer type:

```
socat UDP-DATAGRAM:230.0.0.1:1234,bind=:1234,ip-add-membership=230.0.0.1:127.0.0.1,reuseaddr -
```

This is a hack that uses the complete Ethernet frame, including Ethernet frame header, to communicate data.
It works because the *os* operates in promiscuous mode.


### PXE boot

`make qemu-pxe` or `make qemu-pxe-serial` will boot *os* over PXE.

To use the text mode console, it is neccessary to first set the `MULTIBOOT_VIDEO_INFO` mode value to 1 (EGA text mode).


## Included programs

*os* includes the following programs. They are installed under `/bin`.

| Name | Description |
|---|---|
|alloc |allocate memory |
|box |tiny version of many commands |
|cal |display calendar |
|cat |concatenate files |
|chat |real-time file descriptor chat |
|chmod |change file mode |
|cksum |compute checksum |
|clear |clear the screen |
|cmp |compare files |
|crash |deliberately cause memory access violation |
|date |print date time |
|dd |duplicate data |
|draw |draw random graphics (requires framebuffer) |
|echo |echo |
|env |print environment  |
|expr |evaluate expression |
|false |return failure (1) error code |
|flash |flash the screen |
|forkbomb |fork bomb |
|getty |open tty port with interactive sh |
|grep |print lines matching pattern |
|hello |hello world (assembly) |
|hello++ |hello world (C++) |
|hexdump |hexadecimal dumper |
|hostname |print hostname |
|init |init process |
|kill |kill process |
|ln |create symbolic link |
|ls |list contents of directory |
|mkdir |make directory |
|more |more |
|mount |print mount points |
|mv |rename files |
|pwd |print current working directory |
|reset |reset console |
|rm |remove file |
|rmdir |remove directory |
|sh |shell interpreter |
|sleep |sleep for duration |
|strings |print string sequences |
|testcases |execute standard library test cases |
|tolower |change text to lower case |
|touch |change file timestamp |
|tr |translate characters |
|tree |walk directory tree |
|true |return successful (0) error code |
|truncate |truncate a file |
|uname |print system information |
|unixping |ping the unixserver |
|unixserver |unix sockets demonstration server |
|vi |vi |
|wc |word count |
|xargs |run program with arguments from standard input |

There is no `cp` program; use `cat source > dest` instead.
There is no `ps` program; use `cat /proc/psinfo` instead.
There is no `shutdown` program; use `echo 0 > /dev/power` instead.
The `sh` program supports running programs, pipes, redirection and background processes, but little else.


## History

I wrote my first hobby operating system in 1998 as a learning exercise..
At this time there were very few educational resources on the Internet; many ideas were borrowed from the Linux kernel and another long forgotten website.
A flat memory 32-bit x86 operating system was achieved, with minimal libc and floppy disk driver.
Development was done using DJGPP and tested using Bochs and real-hardware.

In March 2022 I decided to resume the project with the goal of being able to run at least vim and gcc.
A total rewrite ensured, starting with James Molloy's tutorial (<http:///web.archive.org/web/www.jamesmolloy.co.uk/tutorial_html/>), and progressively adding more features.
It took about six weeks of spare time to create a workable system.


## Contact

Peter Ross <pross@xvid.org>

GPG Fingerprint: A907 E02F A6E5 0CD2 34CD 20D2 6760 79C5 AC40 DD6B
