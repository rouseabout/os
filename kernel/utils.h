#include <stdint.h>
#include <time.h>

static inline void outb(uint16_t port, uint8_t value)
{
    asm volatile("outb %0, %1" : : "a" (value), "Nd" (port));
}

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define NB_ELEMS(x) (sizeof(x)/sizeof((x)[0]))

static inline void outw(uint16_t port, uint16_t value)
{
    asm volatile("outw %0, %1" : : "a" (value), "Nd" (port));
}

static inline void outl(uint16_t port, uint32_t value)
{
    asm volatile("outl %0, %1" : : "a" (value), "Nd" (port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    asm volatile("inb %1, %0" : "=a" (value) : "Nd" (port));
    return value;
}

static inline uint16_t inw(uint16_t port)
{
    uint16_t value;
    asm volatile("inw %1, %0" : "=a" (value) : "Nd" (port));
    return value;
}

static inline uint32_t inl(uint16_t port)
{
    uint32_t value;
    asm volatile("inl %1, %0" : "=a" (value) : "Nd" (port));
    return value;
}

int kprintf(const char *, ...);

void khexdump(const void * ptr, unsigned int size);

void panic(const char * reason);

void dump_processes(void);

void * kmalloc(uint32_t size, const char * tag);
uint32_t kmalloc_ap(uint32_t size, uint32_t * phys, const char * tag);

void kfree(void *);

void * krealloc(void * buf, unsigned int size);

void get_absolute_path(char * abspath, int size, const char * path);

void deliver_signal(int pgrp, int signal);

extern struct timespec tnow;

#define KASSERT(x) do { if (!(x)) { kprintf("ASSERT FAILED: " __FILE__ ":%d\n", __LINE__); panic("kassert"); } } while(0)

/* pci */

#define PCI_VENDOR_ID 0
#define PCI_DEVICE_ID 2
#define PCI_COMMAND_ID 4
#define PCI_CLASS 0xB
#define PCI_BAR0 0x10
#define PCI_BAR4 0x20
#define PCI_INTERRUPT_LINE 0x3C

uint16_t pci_read(int bus, int slot, int func, int offset, int size);
void pci_write(int bus, int slot, int func, int offset, int value, int size);
int pci_scan(int (*cb)(void *cntx, int bus, int slot, int func), void * cntx);

typedef void IrqCb(void * context);
extern void * irq_context[16];
extern IrqCb * irq_handler[16];
