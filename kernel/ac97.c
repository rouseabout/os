#include "ac97.h"
#include "ringbuffer.h"
#include "utils.h"
#include <errno.h>
#include <inttypes.h>

#define BUS_PCM_OUT 0x10
#define BUS_PCM_OUT_BUFFER_DESCRIPTOR_LIST_BASE_ADDR (BUS_PCM_OUT + 0x0)
#define BUS_PCM_OUT_CURRENT_INDEX_VALUE (BUS_PCM_OUT + 0x4)
#define BUS_PCM_OUT_LAST_VALID_INDEX (BUS_PCM_OUT + 0x5)
#define BUS_PCM_OUT_STATUS (BUS_PCM_OUT + 0x6)
#define BUS_PCM_OUT_CONTROL (BUS_PCM_OUT + 0xb)

#define BUS_GLOBAL_CONTROL 0x2c

#define MIXER_RESET 0x0
#define MIXER_MASTER_VOLUME 0x2
#define MIXER_PCM_OUT_VOLUME 0x18
#define MIXER_PCM_FRONT_DAC_RATE 0x2c

#define STATUS_DMA_CONTROLLER_HALTED (1 << 0)
#define STATUS_CURRENT_EQUALS_LAST_VALID (1 << 1)
#define STATUS_LAST_VALID_BUFFER_COMPLETION_INTERRUPT (1 << 2)
#define STATUS_BUFFER_COMPLETION_INTERRUPT_STATUS (1 << 3)
#define STATUS_FIFO_ERROR (1 << 4)

#define CONTROL_RUN_PAUSE_BUS_MASTER (1 << 0)
#define CONTROL_RESET_REGISTERS (1 << 1)
#define CONTROL_FIFO_ERROR_INTERRUPT_ENABLE (1 << 3)
#define CONTROL_INTERRUPT_ON_COMPLETION_ENABLE (1 << 4)

#define BUFFER_DESCRIPTOR_BUFFER_UNDERRUN_POLICY (1 << 14)
#define BUFFER_DESCRIPTOR_INTERRUPT_ON_COMPLETION (1 << 15)

#define NB_BUFFER_DESCRIPTORS 32

typedef struct {
    uint32_t addr_phy;
    uint16_t nb_samples;
    uint16_t control;
} __attribute__((packed)) buffer_descriptor;

typedef struct {
    int bus, slot, func;

    unsigned int mixer_base;
    unsigned int bus_base;
    unsigned int irq;

    char * buffer;
    buffer_descriptor * descriptors;
    uint64_t buffer_phy;
    uint64_t descriptors_phy;

    char rb_buffer[4096];
    RingBuffer rb;

    int dma_active;
    int sync;
} AC97Context;

static void ac97_irq(void * opaque)
{
    AC97Context * cntx = opaque;

    uint16_t status = inw(cntx->bus_base + BUS_PCM_OUT_STATUS);

    outw(cntx->bus_base + BUS_PCM_OUT_STATUS, STATUS_LAST_VALID_BUFFER_COMPLETION_INTERRUPT | STATUS_BUFFER_COMPLETION_INTERRUPT_STATUS | STATUS_FIFO_ERROR);

    if (status & STATUS_DMA_CONTROLLER_HALTED)
        cntx->dma_active = 0;
}

static int find_device(void * opaque, int bus, int slot, int func)
{
    AC97Context * s = opaque;
    if (pci_read(bus, slot, func, PCI_CLASS, 2) == 0x0401) {
        s->bus = bus;
        s->slot = slot;
        s->func = func;
        return 1; /* stop */
    }
    return 0;
}

static void mixer_reset(AC97Context * cntx)
{
    outw(cntx->mixer_base + MIXER_RESET, 1);
    outw(cntx->mixer_base + MIXER_MASTER_VOLUME, 100 << 8);
    outw(cntx->mixer_base + MIXER_PCM_OUT_VOLUME, 100 << 8);
}

static void dsp_reset(AC97Context * cntx)
{
    outb(cntx->bus_base + BUS_PCM_OUT_CONTROL, 0);

    outb(cntx->bus_base + BUS_PCM_OUT_CONTROL, CONTROL_RESET_REGISTERS);
    while (inb(cntx->bus_base + BUS_PCM_OUT_CONTROL) & CONTROL_RESET_REGISTERS)
        ;

    outw(cntx->bus_base + BUS_PCM_OUT_STATUS, STATUS_LAST_VALID_BUFFER_COMPLETION_INTERRUPT | STATUS_BUFFER_COMPLETION_INTERRUPT_STATUS | STATUS_FIFO_ERROR);
}

static unsigned int get_sample_rate(AC97Context * cntx)
{
    return inw(cntx->mixer_base + MIXER_PCM_FRONT_DAC_RATE);
}

void * ac97_init()
{
    AC97Context * cntx = kmalloc(sizeof(AC97Context), "ac97-cntx");
    if (!cntx)
        return NULL;

    if (!pci_scan(find_device, cntx)) {
        kfree(cntx);
        return NULL;
    }

    cntx->mixer_base = pci_read(cntx->bus, cntx->slot, cntx->func, PCI_BAR0, 4) & ~0x3;
    cntx->bus_base = pci_read(cntx->bus, cntx->slot, cntx->func, PCI_BAR1, 4) & ~0x3;
    cntx->irq = pci_read(cntx->bus, cntx->slot, cntx->func, PCI_INTERRUPT_LINE, 1);
    kprintf("ac97: mixer_base=0x%x, bus_base=0x%x, irq=%d\n", cntx->mixer_base, cntx->bus_base, cntx->irq);

    cntx->descriptors = (void *)kmalloc_ap(sizeof(buffer_descriptor) * NB_BUFFER_DESCRIPTORS, &cntx->descriptors_phy, "ac97-descriptors"); //FIXME: request phy < 4GiG
    cntx->buffer = (void *)kmalloc_ap(NB_BUFFER_DESCRIPTORS * 4096, &cntx->buffer_phy, "ac97-buffer"); //FIXME: request phy < 4GiG
    memset(cntx->buffer, 0xee, NB_BUFFER_DESCRIPTORS * 4096);

    ringbuffer_init(&cntx->rb, cntx->rb_buffer, sizeof(cntx->rb_buffer));

    irq_context[cntx->irq] = cntx;
    irq_handler[cntx->irq] = ac97_irq;

    uint16_t command = pci_read(cntx->bus, cntx->slot, cntx->func, PCI_COMMAND_ID, 2);
    command &= ~PCI_COMMAND_INTERRUPT_DISABLE;
    command |= PCI_COMMAND_BUS_MASTER | PCI_COMMAND_IO_SPACE;
    pci_write(cntx->bus, cntx->slot, cntx->func, PCI_COMMAND_ID, command, 2);

    uint32_t control = inl(cntx->bus_base + BUS_GLOBAL_CONTROL);
    control |= 0x1 | 0x2; // enable interrupt, cold reset
    outl(cntx->bus_base + BUS_GLOBAL_CONTROL, control);

    mixer_reset(cntx);
    dsp_reset(cntx);

    cntx->dma_active = 0;

    kprintf("ac97: sample_rate=%d\n", get_sample_rate(cntx));

    return cntx;
}

void ac97_poll(void * opaque)
{
    AC97Context * cntx = opaque;

    int req_size = ringbuffer_read_available(&cntx->rb);
    if (!req_size)
        return;

    if (!cntx->sync && req_size < 4096)
        return;

    int cur = inb(cntx->bus_base + BUS_PCM_OUT_CURRENT_INDEX_VALUE);
    int last = inb(cntx->bus_base + BUS_PCM_OUT_LAST_VALID_INDEX);

    if (cntx->dma_active && ((last + 1 ) % NB_BUFFER_DESCRIPTORS) == cur)
        return;

    if (cntx->dma_active)
        last = (last + 1) % NB_BUFFER_DESCRIPTORS;
    else
        memset(cntx->descriptors, 0, sizeof(buffer_descriptor) * NB_BUFFER_DESCRIPTORS);

    int size = ringbuffer_read(&cntx->rb, cntx->buffer + last * 4096, 4096);
    if (size < 4096)
        memset(cntx->buffer + last * 4096 + size, 0, 4096 - size);
    cntx->descriptors[last].addr_phy = cntx->buffer_phy + last * 4096;
    cntx->descriptors[last].nb_samples = 4096 / 2;
    cntx->descriptors[last].control = BUFFER_DESCRIPTOR_INTERRUPT_ON_COMPLETION;

    outb(cntx->bus_base + BUS_PCM_OUT_LAST_VALID_INDEX, last);

    if (!cntx->dma_active) {
        cntx->dma_active = 1;

        outl(cntx->bus_base + BUS_PCM_OUT_BUFFER_DESCRIPTOR_LIST_BASE_ADDR, cntx->descriptors_phy);

        uint8_t control = inb(cntx->bus_base + BUS_PCM_OUT_CONTROL);
        control |= CONTROL_RUN_PAUSE_BUS_MASTER | CONTROL_FIFO_ERROR_INTERRUPT_ENABLE | CONTROL_INTERRUPT_ON_COMPLETION_ENABLE;
        outb(cntx->bus_base + BUS_PCM_OUT_CONTROL, control);
    }
}

static int ac97_write(FileDescriptor * fd, const void * buf, int size)
{
    AC97Context * cntx = fd->priv_data;
    int n = ringbuffer_write(&cntx->rb, buf, size);
    return n ? n : -EAGAIN;
}

static int ac97_write_available(const FileDescriptor * fd)
{
    const AC97Context * cntx = fd->priv_data;
    return ringbuffer_write_available(&cntx->rb);
}

static void ac97_refcount(FileDescriptor * fd, int delta)
{
    AC97Context *cntx = fd->priv_data;
    cntx->sync = 0;
}

static void ac97_close(FileDescriptor * fd)
{
    AC97Context * cntx = fd->priv_data;
    cntx->sync = 1;
}

const DeviceOperations ac97_dio = {
    .write = ac97_write,
    .write_available = ac97_write_available,
    .refcount = ac97_refcount,
    .close = ac97_close,
};
