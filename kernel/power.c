#include "vfs.h"
#include "utils.h"
#include "tty.h"

static int power_write(FileDescriptor * fd, const void * buf_, int size)
{
    outw(0xB004, 0x2000); // Bochs
    outw(0x604, 0x2000); // QEMU
    outw(0x4004, 0x3400); // VirtualBox
    tty_puts("Power off not supported\n");
    return 0;
}

const DeviceOperations power_dio = {.write = power_write};

static int reboot_write(FileDescriptor * fd, const void * buf_, int size)
{
    int status;
    do {
       status = inb(0x64);
    } while(status & 2);
    outb(0x64, 0xfe);
    return 0;
}

const DeviceOperations reboot_dio = {.write = reboot_write};
