#include <stdint.h>
#include <stdlib.h>

typedef struct {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__((packed)) rsdp_descriptor;

enum {
    ACPI_MADT_LOCAL_APIC = 0,
    ACPI_MADT_IO_APIC,
    ACPI_MADT_INTERRUPT_SOURCE_OVERRIDE
};

typedef struct {
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) ics_header;

typedef struct {
    ics_header header;
    uint8_t acpi_processor_uid;
    uint8_t apic_id;
    uint32_t flags;
#define ACPI_LOCAL_APIC_ENABLED 0x1
} __attribute__((packed)) local_apic;

typedef struct {
    ics_header header;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_addr;
    uint32_t global_system_interrupt_base;
} __attribute__((packed)) io_apic;

typedef struct {
    ics_header header;
    uint8_t bus;
    uint8_t source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__((packed)) interrupt_source_override;

const rsdp_descriptor * find_rsdp(uintptr_t addr, size_t size);

void acpi_init(const rsdp_descriptor * rsdp);

int acpi_madt_valid(void);

void acpi_madt_enum(void * opaque, void (*cb)(int type, void * opaque, const void * item));
