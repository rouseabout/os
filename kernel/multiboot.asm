bits 32

MULTIBOOT_PAGE_ALIGN equ 1<<0
MULTIBOOT_MEMORY_INFO equ 1<<1
MULTIBOOT_VIDEO_MODE equ 1<<2
MULTIBOOT_AOUT_KLUDGE equ 0x10000
MULTIBOOT_FLAGS equ MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO | MULTIBOOT_VIDEO_MODE

section .boot.header
align 4

multiboot:
    dd 0x1BADB002
    dd MULTIBOOT_FLAGS
    dd 0x100000000 - (0x1BADB002 + MULTIBOOT_FLAGS)

    ; MULTIBOOT_AOUT_KLUDGE
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0

    ; MULTIBOOT_VIDEO_INFO
    dd 0  ; 0=linear graphics 1=ega text mode
    dd 0
    dd 0
    dd 0 ; bpp

extern start2

global start
start:
    ; grub clears eflags interrupt flag

    mov esp, boot_stack.bottom
    xor ebp, ebp

    push ebx  ; pointer to multiboot_info struct
    push eax  ; multiboot magic number
    call start2
    hlt


section .boot.bss
align 4
boot_stack:
    resb 0x1000
.bottom:
