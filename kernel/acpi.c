#include <stdint.h>
#include <string.h>
#include "acpi.h"
#include "utils.h"

typedef struct {
    uint32_t signature;
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) sdt_header;

typedef struct {
    sdt_header header;
    uint32_t entries[];
} __attribute__((packed)) rsdt;

typedef struct {
    sdt_header header;
    uint32_t lapic_addr;
    uint32_t flags;
    uint8_t structures[];
} __attribute__((packed)) madt;

const rsdp_descriptor * find_rsdp(uintptr_t addr, size_t size)
{
    for (int i = 0; i < size; i += 16) {
        const rsdp_descriptor * rdsp = (void *)(addr + i);
        if (!memcmp(rdsp->signature, "RSD PTR ", 8))
            return rdsp;
    }
    return NULL;
}

static const madt * find_madt(const rsdt * r)
{
    size_t n = r->header.length - sizeof(sdt_header) / sizeof(uint32_t);
    for (int i = 0; i < n; i++) {
        sdt_header * header = alloc_map(r->entries[i], sizeof(sdt_header));
        uint32_t signature = header->signature;
        uint32_t length = header->length;
        //FIXME: unmap header
        if (signature == 0x43495041) //MADT
            return alloc_map(r->entries[i], length);
    }
    return NULL;
}

static const madt * g_madt = NULL;

void acpi_madt_enum(void * opaque, void (*cb)(int type, void * opaque, const void * item))
{
    uint8_t * p = (uint8_t *)g_madt->structures;
    while (p < (uint8_t *)g_madt + g_madt->header.length) {
        const ics_header * header = (const ics_header *)p;
        cb(header->type, opaque, p);
        p += header->length;
    }
}

static void madt_dump(int type, void * opaque, const void * item)
{
    const ics_header * header = (const ics_header *)item;
    if (header->type == ACPI_MADT_LOCAL_APIC) {
        const local_apic * lapic = (const local_apic *)item;
        kprintf("LOCAL_APIC: uid=0x%x id=0x%x, flags=0x%x\n", lapic->acpi_processor_uid, lapic->apic_id, lapic->flags);
    } else if (header->type == ACPI_MADT_IO_APIC) {
        const io_apic * ioapic = (const io_apic *)item;
        kprintf("IO_APIC: io_apic_id=0x%x, io_apic_addr=0x%x, global_system_interrupt_base=0x%x\n", ioapic->io_apic_id, ioapic->io_apic_addr, ioapic->global_system_interrupt_base);
    } else if (header->type == ACPI_MADT_INTERRUPT_SOURCE_OVERRIDE) {
        const interrupt_source_override * iso = (const interrupt_source_override *)item;
        kprintf("INTERRUPT_SOURCE_OVERRIDE: bus=0x%x, source=0x%x, global_system_interrupt=0x%x, flags=0x%x\n", iso->bus, iso->source, iso->global_system_interrupt, iso->flags);
    }
}

void acpi_init(const rsdp_descriptor * rsdp)
{
    kprintf("rsdp: %.*s\n", sizeof(rsdp->oem_id), rsdp->oem_id);
    const rsdt * rsdt_header = alloc_map(rsdp->rsdt_address, sizeof(rsdt));
    size_t rsdt_size = rsdt_header->header.length;
    //FIXME: unmap header

    const rsdt * r = alloc_map(rsdp->rsdt_address, rsdt_size);
    g_madt = find_madt(r);
    if (!g_madt)
        return;

    acpi_madt_enum(NULL, madt_dump);
}

int acpi_madt_valid()
{
    return !!g_madt;
}
