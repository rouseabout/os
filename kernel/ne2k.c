#include "ne2k.h"
#include "ringbuffer.h"
#include "utils.h"
#include <errno.h>
#include <inttypes.h>
#include <unistd.h>
#include <string.h>

#define ENX_CMD  0x00
/* page 0 registers */
#define EN0_STARTPG 0x01 /* start page (WR) */
#define EN0_STOPPG 0x02 /* stop page (WR) */
#define EN0_BOUNDARY 0x03 /* boundary page (RD WR) */
#define EN0_TPSR 0x04 /* transmit start page (WR) */
#define EN0_TCNTLO 0x05 /* transmit byte count 0 (WR) */
#define EN0_TCNTHI 0x06 /* transmit byte count 1 (WR) */
#define EN0_ISR 0x07 /* interrupt status (RD,WR) */
#define EN0_RSARLO 0x08 /* remote start address register 0 (WR ?) */
#define EN0_RSARHI 0x09 /* remote start address register 1 (WR ?) */
#define EN0_RCNTLO 0x0a /* remote byte count register 0 (WR) */
#define EN0_RCNTHI 0x0b /* remote byte count register 1 (WR) */
//#define EN0_RSR 0x0c /* receive status register (RD) */
#define EN0_RXCR 0x0c /* receive configuration register (WR) */
#define EN0_TXCR 0x0d /* transmit configuration register (WR) */
#define EN0_DCFG 0x0e /* data configuration (WR) */
#define EN0_IMR 0x0f /* interrupt mask register (WR) */

/* page 1 registers */
#define EN1_CURPAG 0x07

#define ENX_DATA 0x10
#define ENX_RESET 0x1f

#define TRANSMITBUFFER 0x40 /* first page of tx buffer */

/* COMMAND */
#define E8390_STOP   0x01
#define E8390_START  0x02
#define E8390_TRANS  0x04
#define E8390_RREAD  0x08
#define E8390_RWRITE 0x10
#define E8390_NODMA  0x20
#define E8390_PAGE0  0x00
#define E8390_PAGE1  0x40
#define E8390_PAGE2  0x80

#define BIT_RXCR_SEP  0x01
#define BIT_RXCR_AR   0x02
#define BIT_RXCR_AB   0x04 /* accept broadcast */
#define BIT_RXCR_AM   0x08 /* accept multicast */
#define BIT_RXCR_PRO  0x10 /* promiscuous */
#define BIT_RXCR_MON  0x20

#define BIT_INTERRUPT_PRX  0x01
#define BIT_INTERRUPT_PTX  0x02
#define BIT_INTERRUPT_RXE  0x04
#define BIT_INTERRUPT_TXE  0x08
#define BIT_INTERRUPT_OVW  0x10
#define BIT_INTERRUPT_CNT  0x20
#define BIT_INTERRUPT_RDC  0x40
#define BIT_INTERRUPT_RST  0x80

typedef struct {
    int bus, slot, func;
    unsigned int io_base;
    unsigned int irq;

    char rb_buffer[65536];
    RingBuffer rb;
} NE2KContext;

static void do_read(NE2KContext *cntx, int index, void * buf, int size)
{
    outb(cntx->io_base + EN0_RCNTLO, size & 0xFF);
    outb(cntx->io_base + EN0_RCNTHI, size >> 8);

    outb(cntx->io_base + EN0_RSARLO, index & 0xFF);
    outb(cntx->io_base + EN0_RSARHI, index >> 8);

    for (int i = 0; i < size; i++)
        ((uint8_t *)buf)[i] = inb(cntx->io_base + ENX_DATA);
}

static void ne2k_irq(void * opaque)
{
    NE2KContext * cntx = opaque;

    int status = inb(cntx->io_base + EN0_ISR);

#if 0
    kprintf("ne2k: isr:");
    if (status & BIT_INTERRUPT_PRX) kprintf(" PRX");
    if (status & BIT_INTERRUPT_PTX) kprintf(" PTX");
    if (status & BIT_INTERRUPT_RXE) kprintf(" RXE");
    if (status & BIT_INTERRUPT_TXE) kprintf(" TXE");
    if (status & BIT_INTERRUPT_OVW) kprintf(" OVW");
    if (status & BIT_INTERRUPT_CNT) kprintf(" CNT");
    if (status & BIT_INTERRUPT_RDC) kprintf(" RDC");
    if (status & BIT_INTERRUPT_RST) kprintf(" RST");
    kprintf("\n");
#endif

    if ((status & BIT_INTERRUPT_PRX)) {

        int boundary = inb(cntx->io_base + EN0_BOUNDARY);

        int page = boundary + 1;
        do {
            outb(cntx->io_base + ENX_CMD, E8390_PAGE1);
            int curpag = inb(cntx->io_base + EN1_CURPAG);
            outb(cntx->io_base + ENX_CMD, E8390_PAGE0 | E8390_RREAD | E8390_START);

            if (page == curpag)
                break; /* nothing left to read */

            struct {
                uint8_t status;
                uint8_t next;
                uint16_t size;
            } info;
            do_read(cntx, page << 8, &info, 4);

#if 0
            kprintf("rx: page:%d, [status:%d, next:%d, length:%d]\n", page, info.status, info.next, info.size);
#endif

            if (!(info.status & 1))
                break;

            unsigned int available = ringbuffer_write_available(&cntx->rb);
            if (info.size - 4 <= available) {
                char * data1, * data2;
                unsigned int size1, size2;
                ringbuffer_where(&cntx->rb, RB_WRITE, info.size - 4, &data1, &size1, &data2, &size2);
                do_read(cntx, (page << 8) + 4, data1, size1);
                if (size2)
                    do_read(cntx, (page << 8) + 4 + size1, data2, size2);
                ringbuffer_advance(&cntx->rb, RB_WRITE, info.size - 4);
            }

            page = info.next;

        } while(1);

        outb(cntx->io_base + EN0_BOUNDARY, page - 1);
    }

    outb(cntx->io_base + EN0_ISR, status);
}

static int find_storage(void * opaque, int bus, int slot, int func)
{
    NE2KContext * s = opaque;
    if (pci_read(bus, slot, func, PCI_VENDOR_ID, 2) == 0x10EC && pci_read(bus, slot, func, PCI_DEVICE_ID, 2) == 0x8029) { //RealTek RTL-8029
        s->bus = bus;
        s->slot = slot;
        s->func = func;
        return 1; /* stop */
    }
    return 0;
}

void * ne2k_init()
{
    NE2KContext * cntx = kmalloc(sizeof(NE2KContext), "ne2k-cntx");
    if (!cntx)
        return NULL;

    if (!pci_scan(find_storage, cntx)) {
        kfree(cntx);
        return NULL;
    }

    cntx->io_base = pci_read(cntx->bus, cntx->slot, cntx->func, PCI_BAR0, 4) & ~0x3;
    cntx->irq = pci_read(cntx->bus, cntx->slot, cntx->func, PCI_INTERRUPT_LINE, 1);
    kprintf("ne2k: io_base=0x%x, irq=%d\n", cntx->io_base, cntx->irq);

    irq_context[cntx->irq] = cntx;
    irq_handler[cntx->irq] = ne2k_irq;

    /* reset */
    outb(cntx->io_base + ENX_RESET, inb(cntx->io_base + ENX_RESET));
    while((inb(cntx->io_base + EN0_ISR) & BIT_INTERRUPT_RST) == 0);
    outb(cntx->io_base + EN0_ISR, 0xFF);

    outb(cntx->io_base + ENX_CMD, E8390_PAGE0 | E8390_NODMA | E8390_STOP);
    outb(cntx->io_base + EN0_DCFG, 0x49); // word mode

    outb(cntx->io_base + EN0_RCNTLO, 0);
    outb(cntx->io_base + EN0_RCNTHI, 0);

    outb(cntx->io_base + EN0_IMR, BIT_INTERRUPT_PRX | BIT_INTERRUPT_PTX | BIT_INTERRUPT_RXE | BIT_INTERRUPT_TXE | BIT_INTERRUPT_OVW | BIT_INTERRUPT_CNT);
    outb(cntx->io_base + EN0_ISR, 0xFF);

    outb(cntx->io_base + EN0_RXCR, BIT_RXCR_MON | BIT_RXCR_AB | BIT_RXCR_PRO); // monitor, broadcast, promiscuous
    outb(cntx->io_base + EN0_TXCR, 0x02);

#define NE2K_PAGE_SIZE 256

#define NE2K_RAM_BEGIN  16384
#define NE2K_RAM_END    32768
#define NE2K_RAM_SEND_BEGIN  16384
#define NE2K_RAM_SEND_END    (16384 + 6 * NE2K_PAGE_SIZE)

#define NE2K_RAM_RECV_BEGIN NE2K_RAM_SEND_END
#define NE2K_RAM_RECV_END   NE2K_RAM_END

    //kprintf("ne2k: init: startpg=%d, boundary=%d, stoppg=%d\n", NE2K_RAM_RECV_BEGIN >> 8, NE2K_RAM_RECV_BEGIN >> 8, NE2K_RAM_RECV_END >> 8);

    outb(cntx->io_base + EN0_STARTPG, NE2K_RAM_RECV_BEGIN >> 8);
    outb(cntx->io_base + EN0_STOPPG, NE2K_RAM_RECV_END >> 8);
    outb(cntx->io_base + EN0_BOUNDARY, NE2K_RAM_RECV_BEGIN >> 8);


    outb(cntx->io_base + ENX_CMD, E8390_PAGE1 | E8390_NODMA | E8390_STOP);
    outb(cntx->io_base + EN1_CURPAG, (NE2K_RAM_RECV_BEGIN >> 8) + 1);  //is this right?

    outb(cntx->io_base + ENX_CMD, E8390_PAGE0 | E8390_NODMA | E8390_STOP);

    outb(cntx->io_base + EN0_RCNTLO, 32);
    outb(cntx->io_base + EN0_RCNTHI, 0);
    outb(cntx->io_base + EN0_RSARLO, 0);
    outb(cntx->io_base + EN0_RSARHI, 0);

    ringbuffer_init(&cntx->rb, cntx->rb_buffer, sizeof(cntx->rb_buffer));

    outb(cntx->io_base + ENX_CMD, E8390_PAGE0 | E8390_RREAD | E8390_START);

    //need word mode to read mac addreess
    uint8_t prom[32];
    for (int i=0; i<32; i++)
        prom[i] = inb(cntx->io_base + 0x10);

    kprintf("ne2k: mac=");
    for (int i = 0; i < 6; i++)
        kprintf("%02x", prom[i]);
    kprintf("\n");

    outb(cntx->io_base + EN0_DCFG, 0x58); // byte mode

    return cntx;
}

static int ne2k_write(FileDescriptor * fd, const void * buf_, int size)
{
    NE2KContext * cntx = fd->priv_data;
    const uint8_t * buf = buf_;

    outb(cntx->io_base + EN0_RCNTLO, size & 0xFF);
    outb(cntx->io_base + EN0_RCNTHI, size >> 8);
    outb(cntx->io_base + EN0_RSARLO, 0);
    outb(cntx->io_base + EN0_RSARHI, TRANSMITBUFFER);
    outb(cntx->io_base + ENX_CMD, E8390_PAGE0 | E8390_RWRITE | E8390_START);
    for (int i = 0; i < size; i++)
        outb(cntx->io_base + ENX_DATA, buf[i]);
    outb(cntx->io_base + EN0_TPSR, TRANSMITBUFFER);
    outb(cntx->io_base + EN0_TCNTLO, size & 0xFF);
    outb(cntx->io_base + EN0_TCNTHI, size >> 8);
    outb(cntx->io_base + ENX_CMD, E8390_PAGE0 | E8390_NODMA | E8390_TRANS | E8390_START);

    return size;
}

static int ne2k_read(FileDescriptor * fd, void * buf, int size)
{
    NE2KContext * cntx = fd->priv_data;
    if (!ringbuffer_read_available(&cntx->rb))
        return -EAGAIN;
    return ringbuffer_read(&cntx->rb, buf, size);
}

static int ne2k_read_available(const FileDescriptor * fd)
{
    const NE2KContext * cntx = fd->priv_data;
    return ringbuffer_read_available(&cntx->rb);
}

const DeviceOperations ne2k_dio = {
    .write = ne2k_write,
    .read = ne2k_read,
    .read_available = ne2k_read_available,
};
