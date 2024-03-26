/* multiboot */

#define MULTIBOOT_INFO_MEMORY  0x1
#define MULTIBOOT_INFO_BOOTDEV 0x2
#define MULTIBOOT_INFO_CMDLINE 0x4
#define MULTIBOOT_INFO_MODS    0x8
#define MULTIBOOT_INFO_AOUT    0x10
#define MULTIBOOT_INFO_ELF     0x20
#define MULTIBOOT_INFO_MMAP    0x40
#define MULTIBOOT_INFO_DRIVES  0x80
#define MULTIBOOT_INFO_CONFIG  0x100
#define MULTIBOOT_INFO_BOOTLOADER 0x200
#define MULTIBOOT_INFO_APM_TABLE 0x400
#define MULTIBOOT_INFO_VBE     0x800
#define MULTIBOOT_INFO_FRAMEBUFFER 0x1000

typedef struct {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t name;
    uint32_t reserved;
} multiboot_mod;

typedef struct {
    uint32_t tabsize;
    uint32_t strsize;
    uint32_t addr;
    uint32_t reserved;
} multiboot_syms;

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_higher;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    multiboot_syms syms;
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;

    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;

    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
} multiboot_info;

typedef struct {
    uint32_t size;
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
} __attribute__((packed)) multiboot_memory_map;
