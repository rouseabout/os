#include "ata.h"
#include "utils.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define ATA_PORT_DATA     0x1F0
#define ATA_PORT_ERR      0x1F1
#define ATA_PORT_SC       0x1F2
#define ATA_PORT_LBA_LOW  0x1F3
#define ATA_PORT_LBA_MID  0x1F4
#define ATA_PORT_LBA_HIGH 0x1F5
#define ATA_PORT_DH       0x1F6
#define ATA_PORT_CMD      0x1F7

#define ATA_PRIMARY_CONTROL 0x3F6

#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30
#define ATA_CMD_READ_DMA 0xC8
#define ATA_CMD_WRITE_DMA 0xCA
#define ATA_CMD_ID    0xEC
#define ATA_CMD_FLUSH 0xE7

#define ATA_STATUS_ERR 0x1
#define ATA_STATUS_DRQ 0x8
#define ATA_STATUS_BSY 0x80

typedef struct {
    uint32_t address;
    uint16_t size;
    uint8_t reserved;
    uint8_t eot;
} PRDT;

typedef struct {
    int sectors;
    int bus, slot, func;
    unsigned int bar4;
    PRDT * prdt;
    uint8_t * block;
    uint64_t prdt_phy;
} ATAContext;

static int wait_timeout(int timeout)
{
    int i;
    for (i = 0; i < timeout; i++)
        if (!(inb(ATA_PORT_CMD) & ATA_STATUS_BSY))
            break;
    return i < timeout;
}

/* busy loop until bit is clear */
static void wait_bsy()
{
    while (inb(ATA_PORT_CMD) & ATA_STATUS_BSY) ;
}

static void wait_drq()
{
    while (!(inb(ATA_PORT_CMD) & ATA_STATUS_DRQ)) ;
}

static int init()
{
    outb(ATA_PRIMARY_CONTROL, 4); //SRST
    inb(ATA_PRIMARY_CONTROL);
    outb(ATA_PRIMARY_CONTROL, 0);
    inb(ATA_PRIMARY_CONTROL);
    wait_timeout(1000000);

    outb(ATA_PORT_DH, 0xE0); // master drive
    outb(ATA_PORT_LBA_LOW, 0);
    outb(ATA_PORT_LBA_MID, 0);
    outb(ATA_PORT_LBA_HIGH, 0);

    outb(ATA_PORT_CMD, ATA_CMD_ID);
    if (!wait_timeout(100000))
        return 0;
    if (inb(ATA_PORT_CMD) & ATA_STATUS_ERR)
        return 0;
    if (!(inb(ATA_PORT_CMD) & ATA_STATUS_DRQ))
        return 0;

    kprintf("ata: primary drive found\n");

    uint16_t id[256];
    for (int i = 0; i < sizeof(id)/sizeof(id[0]); i++)
        id[i] = inw(ATA_PORT_DATA);

    if (id[83] & (1 << 10))
        kprintf("ata: lba48\n");

    int lba28_sectors = id[60] | (id[61] << 16);

    return lba28_sectors;
}

#define BMR_COMMAND (s->bar4 + 0)
#define BMR_STATUS  (s->bar4 + 2)
#define BMR_PRDT    (s->bar4 + 4)

static void seek(int lba, int sectors)
{
    wait_bsy();
    outb(ATA_PORT_DH, 0xE0 | (lba >> 24)); // e0: lba28
    outb(ATA_PORT_SC, sectors);
    outb(ATA_PORT_LBA_LOW, lba);
    outb(ATA_PORT_LBA_MID, lba >> 8);
    outb(ATA_PORT_LBA_HIGH, lba >> 16);
}

static void ata_read_sector(ATAContext * s, void * data_, int lba, int sectors)
{
    uint16_t * data = data_;

    seek(lba, sectors);
    if (s->bar4) {
        KASSERT(sectors == 1);
        outb(BMR_COMMAND, 0x00);
        outl(BMR_PRDT, s->prdt_phy);
        outb(BMR_STATUS, inb(BMR_STATUS) | 0x6); // clear error, interrupt bits

        outb(ATA_PORT_CMD, ATA_CMD_READ_DMA);
        outb(BMR_COMMAND, 0x8|0x1); // enable bus master operation

        while(!(inb(BMR_STATUS) & 0x4)) ; // wait for interrupt
        wait_bsy();

        memcpy(data, s->block, 512);

        outb(BMR_STATUS, inb(BMR_STATUS) | 0x6); // clear error, interrupt bits

    } else {
        outb(ATA_PORT_CMD, ATA_CMD_READ);

        for (int i = 0; i < sectors; i++) {
            wait_bsy();
            wait_drq();
            for (int j = 0; j < 256; j++)
                data[i*256 + j] = inw(ATA_PORT_DATA);
        }
    }
}

static void ata_write_sector(ATAContext * s, void * data_, int lba, int sectors)
{
    uint16_t * data = data_;

    seek(lba, sectors);
    if (s->bar4) {
        KASSERT(sectors == 1);
        outb(BMR_COMMAND, 0x00);
        outl(BMR_PRDT, s->prdt_phy);
        outb(BMR_STATUS, inb(BMR_STATUS) | 0x6); // clear error, interrupt bits

        memcpy(s->block, data, 512);

        outb(ATA_PORT_CMD, ATA_CMD_WRITE_DMA);
        outb(BMR_COMMAND, 0x1); // enable bus master operation

        while(!(inb(BMR_STATUS) & 0x4)) ; // wait for interrupt
        wait_bsy();

        outb(BMR_STATUS, inb(BMR_STATUS) | 0x6); // clear error, interrupt bits
    } else {
        outb(ATA_PORT_CMD, ATA_CMD_WRITE);

        for (int i = 0; i < sectors; i++) {
            wait_bsy();
            wait_drq();
            for (int j = 0; j < 256; j++)
                outw(ATA_PORT_DATA, data[i*256 + j]);
        }
    }
}

static void ata_flush()
{
    wait_bsy();
    outb(ATA_PORT_CMD, ATA_CMD_FLUSH);
    wait_bsy();
}

static int find_storage(void * opaque, int bus, int slot, int func)
{
    ATAContext * s = opaque;
    if ((pci_read(bus, slot, func, PCI_CLASS, 2) >> 8) == 0x01) { /* mass storage controller */
        s->bus = bus;
        s->slot = slot;
        s->func = func;
        return 1; /* stop */
    }
    return 0;
}

void * ata_init()
{
    ATAContext * cntx = kmalloc(sizeof(ATAContext), "ata-cntx");
    if (!cntx)
        return NULL;
    cntx->sectors = init();
    if (!cntx->sectors) {
        kfree(cntx);
        return NULL;
    }

    if (pci_scan(find_storage, cntx)) { /* ide controller found */
        int command = pci_read(cntx->bus, cntx->slot, cntx->func, PCI_COMMAND_ID, 2);
        command |= 0x4; /* bus master */
        pci_write(cntx->bus, cntx->slot, cntx->func, PCI_COMMAND_ID, command, 2);

        command = pci_read(cntx->bus, cntx->slot, cntx->func, PCI_COMMAND_ID, 2);
        KASSERT(command & 0x4);

        cntx->bar4 = pci_read(cntx->bus, cntx->slot, cntx->func, PCI_BAR4, 4) & ~0x3;

        cntx->prdt = (PRDT *)kmalloc_ap(sizeof(PRDT), &cntx->prdt_phy, "ata-prdt"); //FIXME: request prdt_phy < 4GiB

        uint64_t phy;
        cntx->block = (uint8_t *)kmalloc_ap(512, &phy, "ata-block"); //FIXME: request phy < 4GiG
        cntx->prdt->address = phy;
        cntx->prdt->size = 512;
        cntx->prdt->eot = 0x80;

    } else
        cntx->bar4 = 0; //fallack to polling mode

    return cntx;
}

static int ata_read_dio(FileDescriptor * fd, void * buf_, int size)
{
    ATAContext * cntx = fd->priv_data;
    uint8_t * buf = buf_;
    int size2 = size;

    int sector  = fd->pos / 512;
    int offset  = fd->pos % 512;
    while (size > 0) {
        char tmp[512];
        ata_read_sector(cntx, tmp, sector, 1);
        int sz = MIN(512 - offset, size);
        memcpy(buf, tmp + offset, sz);

        size -= sz;
        buf += sz;

        sector++;
        offset = 0;
    }

    fd->pos += size2;
    return size2;
}

static int ata_write_dio(FileDescriptor * fd, const void * buf_, int size)
{
    ATAContext * cntx = fd->priv_data;
    const uint8_t * buf = buf_;
    int size2 = size;

    int sector  = fd->pos / 512;
    int offset  = fd->pos % 512;
    while (size > 0) {
        char tmp[512];

        int sz = MIN(512 - offset, size);

        if (offset || offset + sz < 512)
            ata_read_sector(cntx, tmp, sector, 1);

        memcpy(tmp + offset, buf, sz);

        ata_write_sector(cntx, tmp, sector, 1);

        size -= sz;
        buf += sz;

        sector++;
        offset = 0;
    }

    ata_flush();

    fd->pos += size;
    return size2;
}

static off_t ata_lseek_dio(FileDescriptor * fd, off_t offset, int whence)
{
    ATAContext * cntx = fd->priv_data;

    int disk_size = cntx->sectors * 512;

    if (whence == SEEK_END)
        fd->pos = disk_size;
    else if (whence == SEEK_CUR)
        fd->pos = MIN(disk_size, MAX(0, fd->pos + offset));
    else if (whence == SEEK_SET)
        fd->pos = MIN(disk_size, MAX(0, offset));

    return fd->pos;
}

int ata_getsize(void * priv_data)
{
    ATAContext * cntx = priv_data;
    return cntx->sectors * 512;
}

const DeviceOperations ata_dio = {
    .write = ata_write_dio,
    .read  = ata_read_dio,
    .lseek = ata_lseek_dio,
};
