%include "kernel/x86inc.asm"
bits 64
section .boot.header
align 4

extern start2

global start
start:
    cli

    mov esp, boot_stack.bottom
    xor ebp, ebp

    lgdt [gdt64_ptr]

    push 0x8
    push .compat_mode
    retfq

bits 32
.compat_mode:
    mov eax, 0x10
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    mov ss, eax

    mov eax, cr0
    btc eax, 31 ; pg
    mov cr0, eax

    mov ecx, 0xC0000080
    rdmsr
    btc eax, 8 ; lme
    wrmsr

    push esi
    push edi
    call start2
    hlt

section .boot.data
align 8
gdt64:
    GDT_ENTRY32 0, 0, 0, 0
    GDT_ENTRY32 0, 0xFFFFF, 0x9A, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS
    GDT_ENTRY32 0, 0xFFFFF, 0x92, GRAN_32_BIT_MODE | GRAN_4KIB_BLOCKS
.end:

align 8
    times 3 dw 0
gdt64_ptr:
    dw gdt64.end - gdt64 - 1
    dq gdt64

section .boot.bss
align 4
boot_stack:
    resb 0x1000
.bottom:
