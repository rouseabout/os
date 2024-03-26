#include <stdint.h>
#include "utils.h"

uint16_t pci_read(int bus, int slot, int func, int offset, int size)
{
    uint32_t address = (uint32_t)0x80000000 | bus << 16 | slot << 11 | func << 8 | (offset & 0xff);
    outl(0xCF8, address);
    if (size == 4)
        return inl(0xCFC);
    else if (size == 2)
        return inw(0xCFC + (offset & 2));
    else if (size == 1)
        return inb(0xCFC + (offset & 1));
    return 0;
}

void pci_write(int bus, int slot, int func, int offset, int value, int size)
{
    uint32_t address = (uint32_t)0x80000000 | bus << 16 | slot << 11 | func << 8 | (offset & 0xff);
    outl(0xCF8, address);
    if (size == 4)
        outl(0xCFC, value);
    else if (size == 2)
        outw(0xCFC + (offset & 2), value);
    else if (size == 1)
        outb(0xCFC + (offset & 1), value);
}

int pci_scan(int (*cb)(void *cntx, int bus, int slot, int func), void * cntx)
{
    for (int bus = 0; bus < 256; bus++)
        for (int slot = 0; slot < 32; slot++)
            if (pci_read(bus, slot, 0, PCI_VENDOR_ID, 2) != 0xFFFF)
                for (int func = 0; func < 8; func++) {
                    if (pci_read(bus, slot, func, PCI_VENDOR_ID, 2) != 0xFFFF) {
                        /* kprintf("PCI %02x:%02x.%x, vendor=0x%04x, device=0x%04x\n", bus, slot, func,
                               pci_read(bus, slot, func, PCI_VENDOR_ID, 2),
                               pci_read(bus, slot, func, PCI_DEVICE_ID, 2)); */
                        int ret = cb(cntx, bus, slot, func);
                        if (ret)
                            return ret;
                    }
                }
    return 0;
}
