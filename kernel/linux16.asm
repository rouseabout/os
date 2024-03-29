bits 16
org 0x0

SETUP_SECTS equ 1

start: ; linux_params
    db 0x1f1 dup (0)
    db SETUP_SECTS ; @0x1f1: setup sectors
    db 12 dup (0)
    db 0x55, 0xaa ; @0x1fe: boot_flag

start16:
    jmp short .continue ; @0x200: short jump
    dd 0x53726448 ; @0x202: magic
    dd 0x204 ; @0x204: protocol
    db 7 dup (0)
    db 1 ; @0x211: load high bit
    db 2 dup (0)
    dd 0x100000 ; @0x214: code32_start
    db 20 dup (0)
    dd 0x37FFFFFF ; @0x22c: initrd_addr_max
    db 8 dup (0)
    dd 255 ; @0x238: cmdline_size
    db 48 dup (0)

.continue:
    mov esi, ds
    shl esi, 4 ; linux_params

    mov eax, esi
    add eax, gdt - start
    push eax
    push word gdt.end - gdt - 1
    lgdt [esp]

    mov eax, cr0
    or al, 1
    mov cr0, eax

    mov ax, 0x10
    mov ds, ax
    mov ss, ax
    mov es, ax
    jmp dword 0x8:0x100000

align 4
gdt:
    dd 0
    dd 0
.code:
    dw 0xFFFF, 0
    db 0, 10011010b, 11001111b, 0
.data:
    dw 0xFFFF, 0
    db 0, 10010010b, 11001111b, 0
.end:

end:
    db (SETUP_SECTS + 1) * 512 - (end - start) dup (0)
